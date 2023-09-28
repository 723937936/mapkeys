#pragma once

#include <QtWidgets>

struct keymap_pair {
    QLineEdit *lineEdit;
    QComboBox *comboBox;
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = 0);

private slots:
    void readKey(int fd);
    void addLine();
    void onAccept();
    void onKeyboardChanged(int i);

private:
    QVector<keymap_pair> pairs;
    QComboBox *keyboardComboBox;
    QGridLayout *gridLayout;
    QPushButton *addButton;
    QSocketNotifier *notifier = 0;
};
