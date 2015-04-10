#include <QtSingleApplication>
#include <QDir>

#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

#include <Logger.h>
#include <FileAppender.h>

#include "mainwindow.h"

#include <QDebug>

int main(int argc, char *argv[])
{
    QtSingleApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    app.setApplicationName("hippo");
    app.setOrganizationName("HippoNotes");

#if QT_VERSION >= 0x050000
    QString dataDir = QStandardPaths::standardLocations(QStandardPaths::DataLocation).at(0);
#else
    QString dataDir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif

    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    FileAppender* fileAppender = new FileAppender(dataDir + QDir::separator() + "hippo.log");
    Logger::globalInstance()->registerAppender(fileAppender);

    LOG_INFO("Starting the application");
    LOG_INFO("Qt Version: " + QString(qVersion()));

    if (app.isRunning())
             return 0;

    MainWindow mainWin;

    return app.exec();
}
