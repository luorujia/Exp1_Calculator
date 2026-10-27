#-------------------------------------------------
# 实验报告截图驱动工具
# 以真实 QKeyEvent / clicked() 驱动界面，并抓取窗口保存为 PNG
# 用法：ScreenshotTool.exe <输出目录> <场景名>
#-------------------------------------------------

QT       += core gui widgets
CONFIG   += c++17 console

TEMPLATE  = app
TARGET    = ScreenshotTool

SOURCES  += main.cpp \
            ../../mainwindow.cpp

HEADERS  += ../../mainwindow.h

FORMS    += ../../mainwindow.ui

RESOURCES += ../../resources.qrc
