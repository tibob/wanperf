#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Used by QSettings
    a.setOrganizationName("wanperf");
    a.setApplicationName("wanperf");

    MainWindow w;
    w.show();

    return a.exec();
}
