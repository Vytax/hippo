#ifndef SYNCGET_H
#define SYNCGET_H

#include "tbinaryprotocol.h"
#include "note.h"
#include <QObject>
#include <QDateTime>
#include <QList>

class EdamProtocol;

class SyncGet : public QObject
{
    Q_OBJECT
public:
    SyncGet(QObject *parent = 0);

    void sync();
private:
    qint64 firstUSN;
    bool canceled;
    qint64 updatesCount;

    QByteArray createGetFilteredSyncChunkPost(qint32 afterUSN, qint32 maxEntries);
    QByteArray createGetSyncStatePost();
    void GetSyncChunk(qint32 &afterUSN, bool &lastChunk);
    void GetSyncState(qint64 &currentTime, qint64 &fullSyncBefore, qint32 &updateCount, qint64 &uploaded);
    int modificationsCount();
    void updateProgress();

signals:
    void noteUpdated(QString guid);

public slots:
    void cancelSync();
};

#endif // SYNCGET_H
