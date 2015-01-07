#include <QtSingleApplication>


#include "mainwindow.h"

#include <QDebug>

int main(int argc, char *argv[])
{
    QtSingleApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    app.setApplicationName("hippo");
    app.setOrganizationName("HippoNotes");

    if (app.isRunning())
             return 0;

    MainWindow mainWin;

    return app.exec();
}
