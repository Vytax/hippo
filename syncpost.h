#ifndef SYNCPOST_H
#define SYNCPOST_H

#include <QObject>
#include <QQueue>
#include <QHash>

class SyncPost : public QObject
{
    Q_OBJECT
public:
    typedef QHash <QString, QString> t_update;
    typedef QQueue<t_update> t_updates;
    explicit SyncPost(QObject *parent = 0);
    
signals:
    void syncFinished();
    void syncRangeChange(int max);
    void syncProgress(int value);
    void noteGuidChanged(QString oldGuid, QString newGuid);
    
public slots:
    void sync();
    void cancelSync();

private:
    bool canceled;

    int unsyncNotes;
    int Progress;
    qint64 currentUSN;

    t_updates readUpdates();
    void getNoteContent(QString guid);
    void getResourceData(QString guid);
    void updateNote(QString guid);
    void updateTag(QString guid);
    void updateNoteBook(QString guid);

};

#endif // SYNCPOST_H
