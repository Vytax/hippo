#include "note.h"
#include "resource.h"
#include "edamprotocol.h"
#include "Logger.h"
#include "error.h"

#include <QSqlQuery>
#include <QSqlError>

Note::Note(QObject *parent):
    QObject(parent)
{
    updateSequenceNum = 0;
    active = true;
    deleted = QDateTime();
}

Note* Note::fromGUID(QString id) {
    Note *n = new Note();
    if (n->loadFromSQL(id))
        return n;

    delete n;
    return NULL;
}

void Note::loadFromData(hash data)
{
    if (data.contains(1))
        guid = QString::fromUtf8(data[1].toByteArray());
    if (data.contains(2))
        title = QString::fromUtf8(data[2].toByteArray());
    if (data.contains(3))
        content = QString::fromUtf8(data[3].toByteArray());
    if (data.contains(4))
        contentHash = data[4].toByteArray().toHex();
    if (data.contains(5))
        contentLength = data[5].toInt();
    if (data.contains(6))
        created = QDateTime::fromMSecsSinceEpoch(data[6].toLongLong());
    if (data.contains(7))
        updated = QDateTime::fromMSecsSinceEpoch(data[7].toLongLong());
    if (data.contains(8))
        deleted = QDateTime::fromMSecsSinceEpoch(data[8].toLongLong());
    if (data.contains(9))
        active = data[9].toBool();
    if (data.contains(10))
        updateSequenceNum = data[10].toInt();
    if (data.contains(11))
        notebookGuid = QString::fromUtf8(data[11].toByteArray());
    if (data.contains(12)) {
        list l = data[12].toList();
        for (int i=0; i<l.size(); i++){
            QString tguid = QString::fromUtf8(l.at(i).toByteArray());
            tagGuids.append(tguid);
        }
    }
    if (data.contains(13)) {
        list l = data[13].toList();
        QSqlDatabase::database().transaction();
        for (int i=0; i<l.size(); i++){
            hash res = l.at(i).value<hash>();
            Resource *r = new Resource(this, res);
            delete r;
        }
        QSqlDatabase::database().commit();
    }
    if (data.contains(14)) {
        hash attr = data[14].value<hash>();
        readAttributes(attr);
    }
}

void Note::readAttributes(hash data)
{
    if (data.contains(1))
        attributes["subjectDate"] = data[1];

    if (data.contains(10))
        attributes["latitude"] = data[10];

    if (data.contains(11))
        attributes["longitude"] = data[11];

    if (data.contains(12))
        attributes["altitude"] = data[12];

    if (data.contains(13))
        attributes["author"] = data[13];

    if (data.contains(14))
        attributes["sourceURL"] = data[14];

    if (data.contains(15))
        attributes["sourceURL"] = data[15];

    if (data.contains(16))
        attributes["sourceApplication"] = data[16];

    if (data.contains(17))
        attributes["shareDate"] = data[17];

    if (data.contains(18))
        attributes["reminderOrder"] = data[18];

    if (data.contains(19))
        attributes["reminderDoneTime"] = data[19];

    if (data.contains(20))
        attributes["reminderTime"] = data[20];

    if (data.contains(21))
        attributes["placeName"] = data[21];

    if (data.contains(22))
        attributes["contentClass"] = data[22];

    if (data.contains(23))
        attributes["applicationData"] = data[23];

    if (data.contains(24))
        attributes["lastEditedBy"] = data[24];

    if (data.contains(25))
        attributes["classifications"] = data[25];

    if (data.contains(27))
        attributes["creatorId"] = data[27];

    if (data.contains(28))
        attributes["lastEditorId"] = data[28];
}

bool Note::loadFromSQL(QString id)
{
    QSqlQuery noteq;
    noteq.prepare("SELECT title, notebookGuid, active, created, updated, deleted, contentHash, updateSequenceNum FROM notes WHERE guid=:guid");
    noteq.bindValue(":guid", id);
    if (!noteq.exec())
        LOG_ERROR("SQL: " + noteq.lastError().text());

    if (!noteq.next())
         return false;

    guid = id;
    title = noteq.value(0).toString();
    notebookGuid = noteq.value(1).toString();
    active = noteq.value(2).toBool();
    created = QDateTime::fromMSecsSinceEpoch(noteq.value(3).toLongLong());
    updated = QDateTime::fromMSecsSinceEpoch(noteq.value(4).toLongLong());
    deleted = QDateTime::fromMSecsSinceEpoch(noteq.value(5).toLongLong());
    contentHash = noteq.value(6).toString();
    updateSequenceNum = noteq.value(6).toInt();

    return true;
}

void Note::loadTagsSQL()
{
    QSqlQuery noteq;
    noteq.prepare("SELECT guid FROM notesTags WHERE noteGuid=:noteGuid");
    noteq.bindValue(":noteGuid", guid);
    if (!noteq.exec())
        LOG_ERROR("SQL: " + noteq.lastError().text());

    while (noteq.next())
        tagGuids.append(noteq.value(0).toString());
}

void Note::loadAttributesSQL()
{
    QSqlQuery noteq;
    noteq.prepare("SELECT field, value FROM noteAttributes WHERE noteGuid=:noteGuid");
    noteq.bindValue(":noteGuid", guid);
    if (!noteq.exec())
        LOG_ERROR("SQL: " + noteq.lastError().text());

    while (noteq.next())
        attributes[noteq.value(0).toString()] = noteq.value(1);
}

QByteArray Note::createGetContentPost()
{   
    TBinaryProtocol *bin = new TBinaryProtocol();
    bin->writeMessageBegin("getNoteContent", T_CALL, 0);
    bin->writeString(EdamProtocol::GetInstance()->getAuthenticationToken(), 1);
    bin->writeString(guid, 2);
    bin->writeFieldStop();
    QByteArray result = bin->getData();
    delete bin;
    return result;
}

void Note::fetchContent()
{    
    bool ok;
    QByteArray result = EdamProtocol::GetInstance()->getNetworkManager()->postData(EdamProtocol::GetInstance()->getNoteStoreUri(), createGetContentPost(), ok);

    if (!ok) {
        LOG_ERROR("NET ERROR");
        return;
    }

    TBinaryProtocol *bin = new TBinaryProtocol(result);

    QString name;
    TMessageType messageType;
    qint32 seqid;
    bin->readMessageBegin(name, messageType, seqid);
    if (messageType == T_EXCEPTION){
        LOG_ERROR("EDAM: fetchContent failed: unknown result");
        return;
    }    

    hash data = bin->readField();

    if (!data.contains(0))
        Error::readExceptions(data);

    content = QString::fromUtf8(data[0].toByteArray());

    delete bin;

    writeSQLdata();
}

void Note::writeSQL()
{
    QString q("REPLACE INTO notes (guid, title, contentHash, created, updated, deleted, active, updateSequenceNum, notebookGuid)");
    q+= "VALUES (:guid, :title, :contentHash, :created, :updated, :deleted, :active, :updateSequenceNum, :notebookGuid)";
    QSqlQuery query;
    query.prepare(q);
    query.bindValue(":guid", guid);
    query.bindValue(":title", title);
    query.bindValue(":contentHash", contentHash);
    query.bindValue(":created", created.toMSecsSinceEpoch());
    query.bindValue(":updated", updated.toMSecsSinceEpoch());
    query.bindValue(":deleted", deleted.toMSecsSinceEpoch());
    query.bindValue(":active", active);
    query.bindValue(":updateSequenceNum", updateSequenceNum);
    query.bindValue(":notebookGuid", notebookGuid);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());

    if (updateSequenceNum > 0)
        EdamProtocol::GetInstance()->getSyncEngine()->updateUSN(updateSequenceNum);
}
void Note::writeSQLtags() {
    QSqlQuery query;
    query.prepare("DELETE FROM notesTags WHERE noteGuid=:noteGuid");
    query.bindValue(":noteGuid", guid);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());

    for (int i=0; i<tagGuids.size(); i++){
        query.prepare("INSERT INTO notesTags (noteGuid, guid) VALUES (:noteGuid, :guid)");
        query.bindValue(":noteGuid", guid);
        query.bindValue(":guid", tagGuids.at(i));
        if (!query.exec())
            LOG_ERROR("SQL: " + query.lastError().text());
    }
}

void Note::writeSQLdata() {
    if (content.isEmpty())
        return;

    contentHash = QCryptographicHash::hash(content.toUtf8(), QCryptographicHash::Md5).toHex();
    contentLength = content.size();

    QSqlQuery query;
    query.prepare("REPLACE INTO notesContent (hash, content, length) VALUES (:hash, :content, :length)");
    query.bindValue(":hash", contentHash);
    query.bindValue(":content", content);
    query.bindValue(":length", contentLength);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());
}

void Note::writeSQLAttributes() {

    QSqlQuery query;
    query.prepare("DELETE FROM noteAttributes WHERE noteGuid=:noteGuid");
    query.bindValue(":noteGuid", guid);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());

    foreach (const QString &str, attributes.keys()) {
        query.prepare("INSERT INTO noteAttributes (noteGuid, field, value) VALUES (:noteGuid, :field, :value)");
        query.bindValue(":noteGuid", guid);
        query.bindValue(":field", str);
        query.bindValue(":value", attributes[str]);
        if (!query.exec())
            LOG_ERROR("SQL: " + query.lastError().text());
    }
}

QList<int> Note::modifiedFields()
{
    QList<int> result;

    QSqlQuery noteq;
    noteq.prepare("SELECT field FROM noteUpdates WHERE guid=:guid");
    noteq.bindValue(":guid", guid);
    if (!noteq.exec())
        LOG_ERROR("SQL: " + noteq.lastError().text());

    while (noteq.next())
        result.append(noteq.value(0).toInt());

    return result;
}

QByteArray Note::createPushContentPost()
{
    QList<int> modifications = modifiedFields();

    QString action;
    if (modifications.contains(Note::T_NEW))
        action = "createNote";
    else
        action = "updateNote";

    qint64 created_ = created.toMSecsSinceEpoch();
    qint64 updated_ = updated.toMSecsSinceEpoch();
    qint64 deleted_ = deleted.toMSecsSinceEpoch();

    TBinaryProtocol *bin = new TBinaryProtocol();
    bin->writeMessageBegin(action, T_CALL, 0);
    bin->writeString(EdamProtocol::GetInstance()->getAuthenticationToken(), 1);
    bin->writeStructBegin(2);
    if (!modifications.contains(Note::T_NEW))
        bin->writeString(guid, 1);
    bin->writeString(title, 2);
    if (modifications.contains(Note::T_CONTENT))
        bin->writeString(getContent(), 3);
    if (modifications.contains(Note::T_CREATED) && (created_ > 0))
        bin->writeI64(created_, 6);
    if (modifications.contains(Note::T_UPDATED) && (updated_ > 0))
        bin->writeI64(updated_, 7);
    if (modifications.contains(Note::T_DELETED) && (deleted_ > 0))
        bin->writeI64(deleted_, 8);
    if (modifications.contains(Note::T_ACTIVE))
        bin->writeBool(active, 9);
    if (modifications.contains(Note::T_NOTEBOOK_GUID))
        bin->writeString(notebookGuid, 11);
    if (modifications.contains(Note::T_TAG_GUIDS)) {
        loadTagsSQL();
        bin->writeListBegin(12, TBinaryProtocol::T_STRING, tagGuids.size());

        QString tag;
        foreach (tag, tagGuids)
            bin->writeString(tag);
    }
    if (modifications.contains(Note::T_RECOURCES)) {
        QSqlQuery noteq;
        noteq.prepare("SELECT COUNT(*) FROM resources WHERE noteGuid=:guid");
        noteq.bindValue(":guid", guid);
        noteq.exec();
        if (!noteq.exec())
            LOG_ERROR("SQL: " + noteq.lastError().text());

        int count = 0;
        if (noteq.next())
            count = noteq.value(0).toInt();

        if (count > 0) {
            QString q = "SELECT resources.guid, resources.size, resources.mime, resources.new, resourcesData.data, resources.bodyHash, resources.fileName";
            q += " FROM resources LEFT JOIN resourcesData ON resources.bodyHash = resourcesData.hash WHERE resources.noteGuid=:guid";
            noteq.prepare(q);
            noteq.bindValue(":guid", guid);
            if (!noteq.exec())
                LOG_ERROR("SQL: " + noteq.lastError().text());

            bin->writeListBegin(13, TBinaryProtocol::T_STRUCT, count);
            while (noteq.next()) {                              
                bool _new = noteq.value(3).toBool();

                if (_new) {
                    bin->writeStructBegin(3);                       //data struct
                    bin->writeString(noteq.value(5).toString(), 1); //hash
                    bin->writeI32(noteq.value(1).toInt(), 2);       //size
                    bin->writeBinary(noteq.value(4).toByteArray(), 3); //data
                    bin->writeFieldStop();

                    QString fileName = noteq.value(6).toString();
                    if (!fileName.isEmpty()) {
                        bin->writeStructBegin(11);
                        bin->writeString(fileName, 10);
                        bin->writeFieldStop();
                    }
                }
                else
                    bin->writeString(noteq.value(0).toString(), 1); //guid

                bin->writeString(noteq.value(2).toString(), 4); //mime
                bin->writeFieldStop();
            }
        }
    }
    if (modifications.contains(Note::T_ATTRIBUTES)) {
        loadAttributesSQL();
        bin->writeStructBegin(14);

        if (attributes.contains("author"))
            bin->writeString(attributes["author"].toString(), 13);

        if (attributes.contains("sourceURL"))
            bin->writeString(attributes["sourceURL"].toString(), 15);

        bin->writeFieldStop();

    }
    bin->writeFieldStop();
    bin->writeFieldStop();
    QByteArray result = bin->getData();

    delete bin;
    return result;
}

QString Note::getContent()
{
    QSqlQuery noteq;
    noteq.prepare("SELECT content FROM notesContent WHERE hash=:hash");
    noteq.bindValue(":hash", contentHash);
    if (!noteq.exec())
        LOG_ERROR("SQL: " + noteq.lastError().text());

    if (!noteq.next())
        return "";

    return noteq.value(0).toString();
}

QList<QVariantMap> Note::getTags()
{
    QSqlQuery query;
    query.prepare("SELECT notesTags.guid, tags.name FROM notesTags LEFT JOIN tags ON notesTags.guid = tags.guid WHERE notesTags.noteGuid=:noteGuid ORDER BY tags.name COLLATE NOCASE ASC");
    query.bindValue(":noteGuid", guid);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());

    QList<QVariantMap> result;

    while (query.next()) {
        QVariantMap tag;
        tag["guid"] = query.value(0).toString();
        tag["name"] = query.value(1).toString();
        result.append(tag);
    }
    return result;
}

QStringList Note::getTagNames()
{
    QList<QVariantMap> tags = getTags();

    QStringList tagNames;
    QVariantMap tag;

    foreach(tag, tags) {
        tagNames.append(tag["name"].toString());
    }

    return tagNames;
}

QString Note::nodeToText(QDomNode node)
{
    QString result;

    QDomNode n = node.firstChild();
    while(!n.isNull()) {
        if(n.isText()) {
            result = result + " " + n.nodeValue().trimmed();
        } else if (n.hasChildNodes()) {
            result = result + " " + nodeToText(n);
        }

        n = n.nextSibling();
    }

    return result.trimmed();
}

QString Note::getContentTxt()
{
    QString xmldata = getContent();
    if (xmldata.isEmpty())
        return "";

    QDomDocument doc;
    doc.setContent(xmldata);

    return nodeToText(doc.documentElement());
}

QString Note::getContentHash()
{
    return contentHash;
}

QString Note::getTitle()
{
    return title;
}

bool Note::getActive()
{
    return active;
}

QDateTime Note::getCreated()
{
    return created;
}

QDateTime Note::getUpdated()
{
    return updated;
}

void Note::sync()
{
    bool ok;
    QByteArray result = EdamProtocol::GetInstance()->getNetworkManager()->postData(EdamProtocol::GetInstance()->getNoteStoreUri(), createPushContentPost(), ok);

    if (!ok)
        return;

    TBinaryProtocol *bin = new TBinaryProtocol(result);

    QString name;
    TMessageType messageType;
    qint32 seqid;
    bin->readMessageBegin(name, messageType, seqid);
    if (messageType == T_EXCEPTION){
        LOG_ERROR("EDAM: " + name + " failed: unknown result");
        return;
    }

    hash data = bin->readField();
    delete bin;

    if (!data.contains(0))
        Error::readExceptions(data);

    QString oldGuid = guid;

    data = data[0].value<hash>();
    loadFromData(data);

    if (oldGuid != guid) {

        QSqlQuery noteq;
        noteq.prepare("DELETE FROM notes WHERE guid=:guid");
        noteq.bindValue(":guid", oldGuid);
        if (!noteq.exec())
            LOG_ERROR("SQL: " + noteq.lastError().text());

        noteq.prepare("UPDATE syncStatus SET value=:value WHERE option=:option AND value=:oldvalue");
        noteq.bindValue(":value", guid);
        noteq.bindValue(":option", "selNote");
        noteq.bindValue(":oldvalue", oldGuid);
        if (!noteq.exec())
            LOG_ERROR("SQL: " + noteq.lastError().text());

        emit noteGuidChanged(oldGuid, guid);
    }

    QSqlQuery noteq;
    noteq.prepare("DELETE FROM noteUpdates WHERE guid=:guid");
    noteq.bindValue(":guid", oldGuid);
    if (!noteq.exec())
        LOG_ERROR("SQL: " + noteq.lastError().text());

    noteq.prepare("DELETE FROM notesTags WHERE noteGuid=:noteGuid");
    noteq.bindValue(":noteGuid", oldGuid);
    if (!noteq.exec())
        LOG_ERROR("SQL: " + noteq.lastError().text());

    writeSQL();
    writeSQLtags();
}

void Note::editField(int field) {
    editField(guid, field);
}

void Note::editField(QString guid, int field) {
    if (guid.isEmpty())
        return;

    QSqlQuery noteq;
    noteq.prepare("INSERT INTO noteUpdates (guid, field) VALUES (:guid, :field)");
    noteq.bindValue(":guid", guid);
    noteq.bindValue(":field", field);
    if (!noteq.exec())
        LOG_ERROR("SQL: " + noteq.lastError().text());
}

void Note::removeField(int field) {
    removeField(guid, field);
}

void Note::removeField(QString guid, int field) {
    if (guid.isEmpty())
        return;

    QSqlQuery noteq;
    noteq.prepare("DELETE FROM noteUpdates WHERE guid=:guid AND field=:field");
    noteq.bindValue(":guid", guid);
    noteq.bindValue(":field", field);
    if (!noteq.exec())
        LOG_ERROR("SQL: " + noteq.lastError().text());
}

void Note::update(NoteUpdates updates) {
    if (guid.isEmpty()) {
        guid = updates[T_GUID].toString();
        editField(T_NEW);
    }

    if (updates.contains(T_CONTENT)) {
        QString newContent = updates[T_CONTENT].toString();
        if (content != newContent) {
            content = newContent;

            writeSQLdata();

            editField(T_CONTENT);
        }
    }
    if (updates.contains(T_TITLE)) {
        QString newTitle = updates[T_TITLE].toString();
        if (title != newTitle) {
            title = newTitle;
            editField(T_TITLE);
        }
    }
    if (updates.contains(T_CREATED)) {
        QDateTime newCreated = QDateTime::fromMSecsSinceEpoch(updates[T_CREATED].toLongLong());
        if (created != newCreated) {
            created = newCreated;
            editField(T_CREATED);
        }
    }
    if (updates.contains(T_UPDATED)) {
        QDateTime newUpdated = QDateTime::fromMSecsSinceEpoch(updates[T_UPDATED].toLongLong());
        if (updated != newUpdated) {
            updated = newUpdated;
            editField(T_UPDATED);
        }
    }
    if (updates.contains(T_DELETED)) {
        QDateTime newDeleted = QDateTime::fromMSecsSinceEpoch(updates[T_DELETED].toLongLong());
        if (deleted != newDeleted) {
            deleted = newDeleted;
            editField(T_DELETED);
        }
    }
    if (updates.contains(T_NOTEBOOK_GUID)) {
        QString newNotebook = updates[T_NOTEBOOK_GUID].toString();
        if (notebookGuid != newNotebook) {
            notebookGuid = newNotebook;
            editField(T_NOTEBOOK_GUID);
        }
    }
    if (updates.contains(T_ACTIVE)) {
        bool newActive = updates[T_ACTIVE].toBool();
        if (active != newActive) {
            active = newActive;
            editField(T_ACTIVE);
        }
    }
    writeSQL();
}

bool Note::checkContentHash() {
    if (content.isEmpty())
        return false;

    QByteArray hash = QCryptographicHash::hash(content.toUtf8(), QCryptographicHash::Md5);
    return (contentHash == hash.toHex());
}

QString Note::createNewNote(QString notebook) {

    guid = newGUID("notes", "guid");
    title = "New Note";
    content = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE en-note SYSTEM \"http://xml.evernote.com/pub/enml2.dtd\"><en-note></en-note>";
    created = QDateTime::currentDateTime();
    updated = QDateTime::currentDateTime();
    active = true;
    notebookGuid = notebook;

    editField(T_NEW);
    editField(T_CONTENT);
    editField(T_TITLE);
    editField(T_CREATED);
    editField(T_UPDATED);
    editField(T_NOTEBOOK_GUID);

    writeSQLdata();
    writeSQL();

    return guid;
}

void Note::expungeNote(QString id) {
    QSqlQuery query;
    query.prepare("DELETE FROM notes WHERE guid=:guid");
    query.bindValue(":guid", id);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());
    query.clear();

    query.prepare("DELETE FROM notesTags WHERE noteGuid=:noteGuid");
    query.bindValue(":noteGuid", id);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());
    query.clear();

    query.prepare("DELETE FROM resources WHERE noteGuid=:noteGuid");
    query.bindValue(":noteGuid", id);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());
    query.clear();

    query.prepare("DELETE FROM noteUpdates WHERE guid=:guid");
    query.bindValue(":guid", id);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());
    query.clear();
}

QString Note::getNotebookGuid() {
    return notebookGuid;
}

QString Note::getNotebookName() {
    QSqlQuery query;
    query.prepare("SELECT name FROM notebooks WHERE guid=:guid");
    query.bindValue(":guid", notebookGuid);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());

    if (query.next())
        return query.value(0).toString();

    return "";
}

QString Note::getGuid() {
    return guid;
}

bool Note::hasData()
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM notesContent WHERE hash=:hash");
    query.bindValue(":hash", contentHash);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());

    if (query.next())
        return query.value(0).toInt() > 0;

    return false;
}

QVariantMap Note::conflict() {
    QVariantMap result;

    QSqlQuery query;
    query.prepare("SELECT conflictingNotes.contentHash, conflictingNotes.updated, notesContent.content FROM conflictingNotes LEFT JOIN notesContent ON conflictingNotes.contentHash = notesContent.hash WHERE conflictingNotes.guid=:guid");
    query.bindValue(":guid", guid);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());

    if (query.next()) {
        result["contentHash"] = query.value(0);
        result["updated"] = query.value(1);
        result["content"] = query.value(2);
    }

    return result;
}

void Note::dropConflict() {
    QSqlQuery query;
    query.prepare("DELETE FROM conflictingNotes WHERE guid=:guid");
    query.bindValue(":guid", guid);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());
}

qint32 Note::getUSN() {
    return updateSequenceNum;
}

int Note::getSize() {
    return getContent().size();
}

QString Note::getSourceURL() {
    if (attributes.isEmpty())
        loadAttributesSQL();

    if (attributes.contains("sourceURL"))
        return attributes["sourceURL"].toString();

    return "";
}

void Note::updateSourceURL(QString url) {

    loadAttributesSQL();

    if (attributes["sourceURL"].toString() == url)
        return;

    if (url.isEmpty())
        attributes.remove("sourceURL");
    else
        attributes["sourceURL"] = url;

    editField(T_ATTRIBUTES);
    writeSQLAttributes();
}

QString Note::getAuthor() {
    if (attributes.isEmpty())
        loadAttributesSQL();

    if (attributes.contains("author"))
        return attributes["author"].toString();

    return "";
}

void Note::updateAuthor(QString author) {
    loadAttributesSQL();

    if (attributes["author"].toString() == author)
        return;

    if (author.isEmpty())
        attributes.remove("author");
    else
        attributes["author"] = author;

    editField(T_ATTRIBUTES);
    writeSQLAttributes();
}

void Note::writeConflict(QString id, QString hash, qint64 updatedT) {

    if (id.isEmpty() || hash.isEmpty())
        return;

    QSqlQuery query;
    query.prepare("REPLACE INTO conflictingNotes (guid, contentHash, updated) VALUES (:guid, :contentHash, :updated)");
    query.bindValue(":guid", id);
    query.bindValue(":contentHash", hash);
    query.bindValue(":updated", updatedT);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());
}
