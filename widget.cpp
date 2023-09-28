#include "widget.h"
#include <fcntl.h>
#include <glob.h>
#include <linux/input.h>
#include <unistd.h>

static QMap<QString, unsigned int> keynames = {
    {"Caps Lock", KEY_CAPSLOCK},   //
    {"Left Ctrl", KEY_LEFTCTRL},   //
    {"Right Ctrl", KEY_RIGHTCTRL}, //
    {"Left Alt", KEY_LEFTALT},     //
    {"Right Alt", KEY_RIGHTALT},   //
    {"Left Meta", KEY_LEFTMETA},   //
    {"Right Meta", KEY_RIGHTMETA}, //
};

struct keyboard_device {
    QString devfile;
    QString devname;
};

static QVector<keyboard_device> device_list;

static QString get_device_name(const QString &file)
{
    int fd;
    char buf[100];

    fd = open(file.toStdString().c_str(), O_RDONLY);
    if (fd < 0) {
        perror("open");
        return "";
    }
    ioctl(fd, EVIOCGNAME(sizeof(buf)), buf);
    ::close(fd);
    return buf;
}

#define test_bit(key, bits) (bits[key / 8] & key)

static bool is_keyboard(const QString &file)
{
    int fd;
    unsigned char bits[100];

    fd = open(file.toStdString().c_str(), O_RDONLY);
    if (fd < 0) {
        perror("open");
        return false;
    }
    memset(bits, 0, sizeof(bits));
    ioctl(fd, EVIOCGBIT(EV_KEY, KEY_MAX), bits);
    ::close(fd);
    for (unsigned int key : keynames.values())
        if (!test_bit(key, bits))
            return false;
    return true;
}

static void find_keyboard_devices()
{
    glob_t g;

    glob("/dev/input/event*", 0, 0, &g);

    for (size_t i = 0; i < g.gl_pathc; i++) {
        char *devfile = g.gl_pathv[i];
        if (!is_keyboard(devfile))
            continue;
        QString devname = get_device_name(devfile);
        if (devname.isEmpty())
            continue;
        keyboard_device dev;
        dev.devname = devname;
        dev.devfile = devfile;
        device_list.push_back(dev);
    }
}

static struct keymap_pair create_keymap_pair()
{
    QLineEdit *lineEdit = new QLineEdit();
    QComboBox *comboBox = new QComboBox();

    lineEdit->setReadOnly(true);
    lineEdit->setAlignment(Qt::AlignRight);
    lineEdit->setPlaceholderText("输入要修改的键");
    auto keys = keynames.keys();
    keys.sort();
    for (const auto &keyname : keys)
        comboBox->addItem(keyname);

    return {lineEdit, comboBox};
}

Widget::Widget(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("按键修改工具");
    setWindowIcon(QIcon(":/images/mapkeys.png"));
    setMinimumHeight(320);

    find_keyboard_devices();

    if (device_list.isEmpty())
        return;

    keyboardComboBox = new QComboBox();
    for (const auto &dev : device_list)
        keyboardComboBox->addItem(
            QString("%1 (%2)").arg(dev.devname).arg(dev.devfile));
    QObject::connect(keyboardComboBox, SIGNAL(currentIndexChanged(int)), this,
                     SLOT(onKeyboardChanged(int)));

    struct keymap_pair pair = create_keymap_pair();
    pairs.push_back(pair);
    gridLayout = new QGridLayout();
    gridLayout->addWidget(pair.lineEdit, 0, 0);
    gridLayout->addWidget(pair.comboBox, 0, 1);
    gridLayout->setColumnStretch(0, 1);
    gridLayout->setColumnStretch(1, 1);

    addButton = new QPushButton();
    addButton->setText("+");
    QObject::connect(addButton, &QPushButton::clicked, this, &Widget::addLine);

    QVBoxLayout *vboxLayout = new QVBoxLayout();
    vboxLayout->addWidget(keyboardComboBox);
    vboxLayout->addLayout(gridLayout);
    vboxLayout->addWidget(addButton);
    vboxLayout->addStretch();

    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Close);
    buttonBox->button(QDialogButtonBox::Apply)->setText("应用");
    buttonBox->button(QDialogButtonBox::Close)->setText("关闭");

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this,
                     &Widget::onAccept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this,
                     &Widget::close);
    vboxLayout->addWidget(buttonBox);

    setLayout(vboxLayout);
    onKeyboardChanged(0);
}

void Widget::readKey(int fd)
{
    struct input_event e;

    if (read(fd, &e, sizeof(e)) != sizeof(e)) {
        perror("read");
        return;
    }

    if (e.type == EV_MSC) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(qApp->focusWidget());
        if (lineEdit) {
            QString s;
            s.sprintf("%x", e.value);
            lineEdit->setText(s);
        }
    }
}

void Widget::addLine()
{
    struct keymap_pair pair = create_keymap_pair();
    pairs.push_back(pair);
    int rc = gridLayout->rowCount();
    gridLayout->addWidget(pair.lineEdit, rc, 0);
    gridLayout->addWidget(pair.comboBox, rc, 1);
    gridLayout->setColumnStretch(0, 1);
    gridLayout->setColumnStretch(1, 1);
}

void Widget::onAccept()
{
    unsigned int buf[2];

    for (const auto &pair : pairs) {
        QString scancodeText = pair.lineEdit->text();
        if (scancodeText.isEmpty())
            continue;
        buf[0] = scancodeText.toUInt(0, 16);
        buf[1] = keynames[pair.comboBox->currentText()];
        if (ioctl(notifier->socket(), EVIOCSKEYCODE, buf) < 0)
            perror("ioctl");
    }
    close();
}

void Widget::onKeyboardChanged(int i)
{
    if (notifier) {
        ::close(notifier->socket());
        delete notifier;
    }

    int fd = open(device_list[i].devfile.toStdString().c_str(), O_RDWR);
    if (fd < 0) {
        perror("open");
        return;
    }

    notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    QObject::connect(notifier, &QSocketNotifier::activated, this,
                     &Widget::readKey);

    pairs[0].lineEdit->clear();
    pairs[0].comboBox->setCurrentIndex(0);

    while (pairs.size() > 1) {
        auto pair = pairs.back();
        delete pair.lineEdit;
        delete pair.comboBox;
        pairs.pop_back();
    }
}
