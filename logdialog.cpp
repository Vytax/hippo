#include "logdialog.h"
#include "ui_logdialog.h"

#include <QFile>
#include <QDir>

#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

LogDialog::LogDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LogDialog)
{
    ui->setupUi(this);

#if QT_VERSION >= 0x050000
    QString dataDir = QStandardPaths::standardLocations(QStandardPaths::DataLocation).at(0);
#else
    QString dataDir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif


    QFile file(dataDir + QDir::separator() + "hippo.log");

    if(!file.open(QIODevice::ReadOnly))
        return;

    ui->textBrowser->setText(file.readAll().trimmed());
    file.close();

    QTextCursor cursor = ui->textBrowser->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.movePosition(QTextCursor::StartOfLine);
    ui->textBrowser->setTextCursor(cursor);
}

LogDialog::~LogDialog()
{
    delete ui;
}

void LogDialog::appendMessage(QString msg) {
    ui->textBrowser->append(msg);
}
