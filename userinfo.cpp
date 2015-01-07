#include "userinfo.h"
#include "ui_userinfo.h"
#include "edamprotocol.h"
#include "sql.h"

UserInfo::UserInfo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UserInfo)
{
    ui->setupUi(this);    

    QString username = sql::readSyncStatus("user.username").toString();
    QString name = sql::readSyncStatus("user.name").toString();
    int privilege = sql::readSyncStatus("user.privilege").toInt();
    qint64 uploadLimit = sql::readSyncStatus("user.accounting.uploadLimit").toLongLong();
    qint64 uploadLimitEnd_ = sql::readSyncStatus("user.accounting.uploadLimitEnd").toLongLong();
    qint64 uploaded = sql::readSyncStatus("uploaded").toLongLong();

    if (!username.isEmpty())
        ui->username->setText(username);

    if (!name.isEmpty())
        ui->name->setText(name);

    if (privilege == 1)
        ui->privilege->setText("NORMAL");

    if (privilege == 3)
        ui->privilege->setText("PREMIUM");

    if ((uploadLimit > 0) && (uploadLimitEnd_ > 0)) {
        QDateTime uploadLimitEnd = QDateTime::fromMSecsSinceEpoch(uploadLimitEnd_);
        ui->quotaProgress->setMaximum(1000);
        quint64 quotaleft= uploadLimit - uploaded;
        int quotaP = (int)((((double)quotaleft) / uploadLimit) * 1000);
        ui->quotaProgress->setValue(quotaP);

        QString quotaText = size_human(quotaleft) + " from " + size_human(uploadLimit) + " of your quota left.";
        quotaText += "<br />Upload limit expires on: " + uploadLimitEnd.toString("yyyy.MM.dd");
        ui->quotaText->setText(quotaText);
    }
}

UserInfo::~UserInfo()
{
    delete ui;
}

QString UserInfo::size_human(qint64 size)
{
     QStringList list;
     list << "KB" << "MB" << "GB" << "TB";

     QStringListIterator i(list);
     QString unit("bytes");

     while(size >= 1024 && i.hasNext())
      {
         unit = i.next();
         size /= 1024;
     }
     return QString().setNum(size)+" "+unit;
}
