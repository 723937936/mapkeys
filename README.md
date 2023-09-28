# 按键修改工具


## 构建

```
$ sudo apt install qt5-default
$ git clone https://github.com/723937936/mapkeys.git
$ cd mapkeys
$ mkdir build
$ cd build
$ qmake ..
$ make -j8
```

## 运行

```
$ sudo ./mapkeys
```

或者将你的账户添加到input组

```
$ sudo add your_account input
```

**将账户添加到组，需要登出后重新登陆才能生效**


## ubuntu20.04上的截图

![这是图片](/snapshot.png "Magic Gardens")

* 顶部的下拉框是选择键盘的
* 左边输入框是要修改的键的scancode，按键自动识别
* 右边的下拉框是要映射成哪个键

## 缺点

* 重启后按键失效，需要重新映射
