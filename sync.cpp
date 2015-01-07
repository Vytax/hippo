#include "sync.h"
#include "sql.h"
#include "edamprotocol.h"
#include <QSqlQuery>

#include <QDebug>

Sync::Sync(QObject *parent) :
    QObject(parent)
{
    started = false;
    currentUSN = sql::readSyncStatus("currentUSN", 0).toLongLong();

    get = new SyncGet(this);
    post = new SyncPost(this);

    connect(get, SIGNAL(syncStarted(int)), this, SIGNAL(syncStarted(int)));
    connect(get, SIGNAL(syncProgress(int)), this, SIGNAL(syncProgress(int)));
    connect(get, SIGNAL(syncFinished()), post, SLOT(sync()));
    connect(post, SIGNAL(syncFinished()), this, SIGNAL(syncFinished()));
    connect(post, SIGNAL(syncRangeChange(int)), this, SIGNAL(syncRangeChange(int)));
    connect(post, SIGNAL(syncProgress(int)), this, SIGNAL(syncProgress(int)));
    connect(post, SIGNAL(syncRangeChange(int)), this, SIGNAL(syncStarted(int)));
    connect(this, SIGNAL(syncFinished()), this, SLOT(getUser()));
    connect(post, SIGNAL(noteGuidChanged(QString,QString)), this, SIGNAL(noteGuidChanged(QString,QString)));
    connect(post, SIGNAL(syncFinished()), this, SLOT(finished()));
}

void Sync::sync()
{
    qDebug() << "Sync::sync()" << started;
    if (started)
        return;

    started = true;
    get->sync();
}

void Sync::cancelSync()
{
    canceled = true;
    get->cancelSync();
    post->cancelSync();
}

qint64 Sync::getUSN()
{
    return currentUSN;
}

void Sync::updateUSN(qint64 usn)
{
    if (usn <= currentUSN)
        return;

    qDebug() << "USN change:" << currentUSN << usn;
    currentUSN = usn;
    writeUSN();
}

void Sync::resetUSN()
{
    currentUSN = 0;
    writeUSN();
}

void Sync::writeUSN()
{
    sql::updateSyncStatus("currentUSN", currentUSN);
}

QByteArray Sync::createGetUserPost()
{
    TBinaryProtocol *bin = new TBinaryProtocol();
    bin->writeMessageBegin("getUser", T_CALL, 0);
    bin->writeString(EdamProtocol::GetInstance()->getAuthenticationToken(), 1);
    bin->writeFieldStop();
    QByteArray result = bin->getData();
    delete bin;
    return result;
}

void Sync::getUser()
{
    qDebug() << "getUser";

    qint64 uploadLimitEnd = sql::readSyncStatus("user.accounting.uploadLimitEnd").toLongLong();

    if (uploadLimitEnd > QDateTime::currentMSecsSinceEpoch())
        return;

    bool ok;
    QByteArray result = EdamProtocol::GetInstance()->getNetworkManager()->postData(EdamProtocol::GetInstance()->getUserStoreUri(), createGetUserPost(), ok);

    if (!ok) {
        return;
    }

    TBinaryProtocol *bin = new TBinaryProtocol(result);

    QString name;
    TMessageType messageType;
    qint32 seqid;
    bin->readMessageBegin(name, messageType, seqid);
    if (messageType == T_EXCEPTION){
        qDebug() << "Error:" << "Authentification failed: unknown result";
        return;
    }
    qDebug() << name << messageType << seqid;

    hash data = bin->readField();
    delete bin;

    if (!data.contains(0))
        return;

    hash user = data[0].value<hash>();

    if (user.contains(1))
        sql::updateSyncStatus("user.id", user[1].toInt());

    if (user.contains(2))
        sql::updateSyncStatus("user.username", QString::fromUtf8(user[2].toByteArray()));

    if (user.contains(4))
        sql::updateSyncStatus("user.name", QString::fromUtf8(user[4].toByteArray()));

    if (user.contains(7))
        sql::updateSyncStatus("user.privilege", user[7].toInt());

    if (user.contains(13))
        sql::updateSyncStatus("user.active", user[13].toString());

    if (user.contains(14))
        sql::updateSyncStatus("user.shardId", QString::fromUtf8(user[14].toByteArray()));

    if (user.contains(16)) {
        hash accounting = user[16].value<hash>();

        if (accounting.contains(1))
            sql::updateSyncStatus("user.accounting.uploadLimit", accounting[1].toLongLong());

        if (accounting.contains(2))
            sql::updateSyncStatus("user.accounting.uploadLimitEnd", accounting[2].toLongLong());

        if (accounting.contains(3))
            sql::updateSyncStatus("user.accounting.uploadLimitNextMonth", accounting[3].toLongLong());
    }

    qDebug() << user.keys() << QString::fromUtf8(user[2].toByteArray());
}

void Sync::finished() {
    started = false;

    qDebug() << "Sync::finished()";
}
