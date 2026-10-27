#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Calculator"));
    app.setApplicationVersion(QStringLiteral("1.0"));

    MainWindow w;
    w.show();
    return app.exec();
}
