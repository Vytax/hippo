#ifndef SYNC_H
#define SYNC_H

#include <QObject>
#include "syncget.h"
#include "syncpost.h"

class SyncGet;
class SyncPost;

class Sync : public QObject
{
    Q_OBJECT
public:
    explicit Sync(QObject *parent = 0);

    void sync();
    qint64 getUSN();
    void updateUSN(qint64 usn);
    void resetUSN();
    void updateProgress(int prog, int step);
signals:
    void syncFinished();
    void syncStarted(int count);
    void syncProgress(int value);
    void syncRangeChange(int max);
    void noteGuidChanged(QString oldGuid, QString newGuid);
    
public slots:
    void cancelSync();

private slots:
    void getUser();
    void finished();

private:
    bool canceled;
    bool started;
    qint64 currentUSN;

    SyncGet *get;
    SyncPost *post;

    void writeUSN();
    QByteArray createGetUserPost();
    
};

#endif // SYNC_H
