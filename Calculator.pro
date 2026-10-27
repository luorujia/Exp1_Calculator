#-------------------------------------------------
# 实验1：带键盘事件的计算器
# Qt 6.x  /  MinGW 64-bit
#-------------------------------------------------

QT       += core gui widgets

CONFIG   += c++17

TEMPLATE  = app
TARGET    = Calculator

SOURCES  += main.cpp \
            mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

RESOURCES += resources.qrc

# 默认部署规则
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
