QT += core gui widgets
CONFIG += c++14
DEFINES += QT_DEPRECATED_WARNINGS

RESOURCES += mapkeys.qrc

SOURCES += $$files(*.cpp)
HEADERS += $$files(*.h)
