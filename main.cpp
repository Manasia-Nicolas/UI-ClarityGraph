#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    // Avoid querying desktop portals for settings (prevents DBus warnings)
    // QApplication::setDesktopSettingsAware(false);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
