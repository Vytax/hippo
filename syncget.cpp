#include "syncget.h"
#include "notebook.h"
#include "tag.h"
#include "resource.h"
#include "edamprotocol.h"

#include <QSqlQuery>
#include <QEventLoop>

#include <QDebug>

SyncGet::SyncGet(QObject *parent):
        QObject(parent)
{
    canceled = false;
}

QByteArray SyncGet::createGetFilteredSyncChunkPost(qint32 afterUSN, qint32 maxEntries)
{
    TBinaryProtocol *bin = new TBinaryProtocol();
    bin->writeMessageBegin("getFilteredSyncChunk", T_CALL, 0);
    bin->writeString(EdamProtocol::GetInstance()->getAuthenticationToken(), 1);
    bin->writeI32(afterUSN, 2);
    bin->writeI32(maxEntries, 3);
    bin->writeStructBegin(4);
    bin->writeBool(true, 1); // includeNotes
    bin->writeBool(true, 2); // includeNoteResources
    bin->writeBool(false, 3); // includeNoteAttributes
    bin->writeBool(true, 4); // includeNotebooks
    bin->writeBool(true, 5); // includeTags
    bin->writeBool(false, 6);// includeSearches
    bin->writeBool(true, 7); // includeResources
    bin->writeBool(false, 8);// includeLinkedNotebooks
    bin->writeBool(true, 9); // includeExpunged
    bin->writeBool(false, 10);//includeNoteApplicationDataFullMap
    //bin->writeString("", 11); //requireNoteContentClass
    bin->writeBool(false, 12);//includeResourceApplicationDataFullMap
    bin->writeBool(false, 13);//includeNoteResourceApplicationDataFullMap
    bin->writeFieldStop();
    bin->writeFieldStop();
    QByteArray result = bin->getData();
    delete bin;
    return result;
}

void SyncGet::GetSyncChunk(qint32 afterUSN, qint32 maxEntries)
{
    qDebug() << "GetSyncChunk "  << afterUSN;
    QByteArray postData = createGetFilteredSyncChunkPost(afterUSN, maxEntries);

    bool ok;
    QByteArray result = EdamProtocol::GetInstance()->getNetworkManager()->postData(EdamProtocol::GetInstance()->getNoteStoreUri(), postData, ok);

    if (!ok) {
        return;
    }

    TBinaryProtocol *bin = new TBinaryProtocol(result);

    QString name;
    TMessageType messageType;
    qint32 seqid;
    bin->readMessageBegin(name, messageType, seqid);
    if (messageType == T_EXCEPTION){
        qDebug() << "Error:" << "checkVersion failed: unknown result";
        return;
    }
    qDebug() << name << messageType << seqid;

    hash data = bin->readField();
    delete bin;
    hash SyncChunk = data[0].value<hash>();
    qDebug() << SyncChunk.keys();

    //currentTime = SyncChunk[1].toLongLong();
    //currentUSN = SyncChunk[2].toInt();
    qint32 updateCount = SyncChunk[3].toInt();

    qDebug() <<  SyncChunk[2].toInt();
    qDebug() << EdamProtocol::GetInstance()->getSyncEngine()->getUSN() << updateCount;

    if (SyncChunk.contains(4)){
        list l = SyncChunk[4].toList();
        for (int i = 0; i< l.size(); i++){
            qDebug() << "note ------------------";
            hash n = l.at(i).value<hash>();
            qDebug() << n.keys();

            Note *note = new Note(this);

            if (n.contains(1)) {
                QString guid = QString::fromUtf8(n[1].toByteArray());
                note->loadFromSQL(guid);
            }            

            QList<int> modifiedFields = note->modifiedFields();
            if (!modifiedFields.isEmpty()) {
                qDebug() << "CONFLICT!";
                
                QDateTime updated = QDateTime::fromMSecsSinceEpoch(0);
                if (n.contains(7))
                    updated = QDateTime::fromMSecsSinceEpoch(n[7].toLongLong());

                qint32 usn = 0;                
                if (n.contains(10))
                    usn = n[10].toInt();

                if (modifiedFields.contains(Note::T_CONTENT) && n.contains(4)) {
                    QString contentHash = n[4].toByteArray().toHex();

                    if (contentHash.compare(note->getContentHash(), Qt::CaseInsensitive) != 0) {
                        Note::writeConflict(note->getGuid(), note->getContentHash(),note->getUpdated().toMSecsSinceEpoch());
                        qDebug() << "Content!";
                    }
                    n.remove(7); // Updated
                    n.remove(8); // Deleted
                }

                qDebug() << "Updated" << updated;
                qDebug() << "note->getUpdated()" << note->getUpdated();


                if (updated < note->getUpdated()) {

                    if (modifiedFields.contains(Note::T_TITLE))
                        n.remove(2);
                    
                    if (modifiedFields.contains(Note::T_DELETED))
                        n.remove(8);
                    
                    if (modifiedFields.contains(Note::T_ACTIVE))
                        n.remove(9);
                    
                    if (modifiedFields.contains(Note::T_NOTEBOOK_GUID))
                        n.remove(11);
                    
                    if (modifiedFields.contains(Note::T_TAG_GUIDS)) {
                        n.remove(12);
                        note->loadTagsSQL();
                    }
                } else {

                    if (n.contains(2))
                        note->removeField(Note::T_TITLE);
                    if (n.contains(8))
                        note->removeField(Note::T_DELETED);
                    if (n.contains(9))
                        note->removeField(Note::T_ACTIVE);
                    if (n.contains(11))
                        note->removeField(Note::T_NOTEBOOK_GUID);
                    if (n.contains(12))
                        note->removeField(Note::T_TAG_GUIDS);
                }

            }

            if (!n.isEmpty()){
                note->loadFromData(n);

                note->writeSQL();
                note->writeSQLtags();
                note->writeSQLAttributes();
            }
            delete note;
            updateProgress();
        }        
    }
    if (SyncChunk.contains(5)){
        list l = SyncChunk[5].toList();
        for (int i = 0; i< l.size(); i++){
            hash n = l.at(i).value<hash>();

            NoteBook *nb = new NoteBook();

            if (n.contains(1)) {
                QString guid = QString::fromUtf8(n[1].toByteArray());
                nb->loadFromSQL(guid);
            }

            nb->loadFromData(n);
            nb->toSQL();
            delete nb;
            updateProgress();
        }
    }
    if (SyncChunk.contains(6)){
        list l = SyncChunk[6].toList();
        for (int i = 0; i< l.size(); i++){
            hash n = l.at(i).value<hash>();
            Tag *tag = new Tag(this);

            if (n.contains(1)) {
                QString guid = QString::fromUtf8(n[1].toByteArray());
                tag->loadFromSQL(guid);
            }
            tag->loadFromData(n);
            tag->toSQL();
            delete tag;
            updateProgress();
        }
    }
    if (SyncChunk.contains(8)){
        list l = SyncChunk[8].toList();
        for (int i = 0; i< l.size(); i++){
            hash n = l.at(i).value<hash>();
            Resource *res = new Resource(this, n);
       //     if (!res->hasData()) {
       //         res->getContent();
       //     }
            delete res;
            updateProgress();
        }
    }
    if (SyncChunk.contains(9)){
        list l = SyncChunk[9].toList();
        for (int i = 0; i< l.size(); i++){
            QString guid = l.at(i).toString();
            Note::expungeNote(guid);
        }
    }
    if (SyncChunk.contains(10)){
        list l = SyncChunk[10].toList();
        for (int i = 0; i< l.size(); i++){
            QString guid = l.at(i).toString();
            NoteBook::expungeNotebook(guid);
        }
    }
    if (SyncChunk.contains(11)){
        list l = SyncChunk[11].toList();
        for (int i = 0; i< l.size(); i++){
            QString guid = l.at(i).toString();
            Tag::expungeTag(guid);
        }
    }
    if (SyncChunk.contains(2)) {
        EdamProtocol::GetInstance()->getSyncEngine()->updateUSN(SyncChunk[2].toInt());
    }

    updateProgress();
}

QByteArray SyncGet::createGetSyncStatePost()
{
    TBinaryProtocol *bin = new TBinaryProtocol();
    bin->writeMessageBegin("getSyncState", T_CALL, 0);
    bin->writeString(EdamProtocol::GetInstance()->getAuthenticationToken(), 1);
    bin->writeFieldStop();
    QByteArray result = bin->getData();
    delete bin;
    return result;
}

void SyncGet::GetSyncState(qint64 &currentTime, qint64 &fullSyncBefore, qint32 &updateCount, qint64 &uploaded)
{
    qDebug() << "GetSyncState";

    bool ok;
    QByteArray result = EdamProtocol::GetInstance()->getNetworkManager()->postData(EdamProtocol::GetInstance()->getNoteStoreUri(), createGetSyncStatePost(), ok);

    if (!ok)
        return;

    TBinaryProtocol *bin = new TBinaryProtocol(result);

    QString name;
    TMessageType messageType;
    qint32 seqid;
    bin->readMessageBegin(name, messageType, seqid);
    if (messageType == T_EXCEPTION){
        qDebug() << "Error:" << "GetSyncState failed: unknown result";
        return;
    }
    qDebug() << name << messageType << seqid;

    hash data = bin->readField();
    delete bin;

    if (!data.contains(0)) {
        qDebug() << "KLAIDA!" << data.keys();

        if (data.contains(1)) {
            data = data[1].value<hash>();
            qDebug() << QString::fromUtf8(data[1].toByteArray()) << QString::fromUtf8(data[2].toByteArray());
        }
        if (data.contains(2)) {
            data = data[2].value<hash>();
            qDebug() << data[1].toInt() << QString::fromUtf8(data[2].toByteArray());
        }
        if (data.contains(3)) {
            data = data[3].value<hash>();
            qDebug() << QString::fromUtf8(data[1].toByteArray()) << QString::fromUtf8(data[2].toByteArray());
        }

        return;
    }

    hash s = data[0].value<hash>();

    currentTime = s[1].toLongLong();
    fullSyncBefore = s[2].toLongLong();
    updateCount = s[3].toInt();
    uploaded = s[4].toLongLong();
}

void SyncGet::sync()
{
    qDebug() << "SyncGet::sync()";
    canceled = false;
    qint64 LastFullSyncDate = sql::readSyncStatus("LastFullSyncDate", 0).toLongLong();
    qint64 currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch();

    qint64 fullSyncBefore = 0;
    qint32 updateCount = 0;
    qint64 uploaded;

    GetSyncState(currentTime, fullSyncBefore, updateCount, uploaded);

    if (updateCount == 0) {
        emit syncFinished();
        return;
    }

    sql::updateSyncStatus("uploaded", uploaded);

    bool fullSync = false;
    if ((modificationsCount() == 0) && ((fullSyncBefore > LastFullSyncDate) || (EdamProtocol::GetInstance()->getSyncEngine()->getUSN() > updateCount))) {
        LastFullSyncDate = currentTime;
        EdamProtocol::GetInstance()->getSyncEngine()->resetUSN();
        EdamProtocol::GetInstance()->getDB()->dropTables();
        fullSync = true;
    }

    if ((EdamProtocol::GetInstance()->getSyncEngine()->getUSN() >= updateCount)){
        emit syncFinished();
        return;
    }

    firstUSN = EdamProtocol::GetInstance()->getSyncEngine()->getUSN();

    emit syncStarted(updateCount - EdamProtocol::GetInstance()->getSyncEngine()->getUSN());

    qint32 chunkSize = 500;
    for (qint32 usn = EdamProtocol::GetInstance()->getSyncEngine()->getUSN(); usn < updateCount; usn += chunkSize) {

        if (canceled)
            break;

        GetSyncChunk(usn, chunkSize);
    }

    if (fullSync)
        sql::updateSyncStatus("LastFullSyncDate", LastFullSyncDate);

    emit syncFinished();
}

void SyncGet::cancelSync()
{
    canceled = true;
}

void SyncGet::updateProgress()
{
    emit syncProgress(EdamProtocol::GetInstance()->getSyncEngine()->getUSN() - firstUSN);
}

int SyncGet::modificationsCount()
{
    QSqlQuery query;
    int count = 0;

    query.exec("SELECT COUNT(*) FROM notebookUpdates");
    if (query.next())
        count += query.value(0).toInt();

    query.clear();
    query.exec("SELECT COUNT(*) FROM tagsUpdates");
    if (query.next())
        count += query.value(0).toInt();

    query.clear();
    query.exec("SELECT COUNT(*) FROM noteUpdates");
    if (query.next())
        count += query.value(0).toInt();

    return count;
}


