#include "tag.h"
#include "edamprotocol.h"
#include <QSqlQuery>

#include <QDebug>
#include <QSqlError>

Tag::Tag(QObject *parent) :
    QObject(parent)
{
    updateSequenceNum = 0;
}

void Tag::loadFromData(hash data) {
    qDebug() << "Tag" << data.keys();
    if (data.contains(1))
        guid = QString::fromUtf8(data[1].toByteArray());
    if (data.contains(2))
        name = QString::fromUtf8(data[2].toByteArray());
    if (data.contains(3))
        parentGuid = QString::fromUtf8(data[3].toByteArray());
    if (data.contains(4))
        updateSequenceNum = data[4].toInt();

    qDebug() << guid << name << parentGuid << updateSequenceNum;
}

void Tag::toSQL()
{
    QString q("REPLACE INTO tags (guid, name, parentGuid, updateSequenceNum)");
    q+= "VALUES (:guid, :name, :parentGuid, :updateSequenceNum)";
    QSqlQuery query;
    query.prepare(q);
    query.bindValue(":guid", guid);
    query.bindValue(":name", name);
    query.bindValue(":parentGuid", parentGuid);
    query.bindValue(":updateSequenceNum", updateSequenceNum);
    query.exec();

    if (updateSequenceNum > 0)
        EdamProtocol::GetInstance()->getSyncEngine()->updateUSN(updateSequenceNum);
}

bool Tag::loadFromSQL(QString id)
{
    QSqlQuery tagq;
    tagq.prepare("SELECT name, parentGuid, updateSequenceNum FROM tags WHERE guid=:guid");
    tagq.bindValue(":guid", id);
    tagq.exec();

    if (!tagq.next())
         return false;

    guid = id;
    name = tagq.value(0).toString();
    parentGuid = tagq.value(1).toString();
    updateSequenceNum = tagq.value(2).toInt();

    return true;
}

void Tag::editField(int field) {
    editField(guid, field);
}

void Tag::editField(QString id, int field) {
    if (id.isEmpty())
        return;

    QSqlQuery query;
    query.prepare("INSERT INTO tagsUpdates (guid, field) VALUES (:guid, :field)");
    query.bindValue(":guid", id);
    query.bindValue(":field", field);
    query.exec();
}

void Tag::update(TagUpdates updates) {
    if (guid.isEmpty()) {
        if (!updates.contains(T_GUID))
            return;
        guid = updates[T_GUID].toString();
        editField(T_GUID);
    }
    if (updates.contains(T_NAME)) {
        name = updates[T_NAME].toString();
        editField(T_NAME);
    }
    if (updates.contains(T_PARENT)) {
        parentGuid = updates[T_PARENT].toString();
        editField(T_PARENT);
    }
    toSQL();
}

QByteArray Tag::createPushContentPost() {
    QSqlQuery query;
    query.prepare("SELECT field FROM tagsUpdates WHERE guid=:guid");
    query.bindValue(":guid", guid);
    query.exec();

    QList<int> modifiedFields;

    while (query.next())
        modifiedFields.append(query.value(0).toInt());

    QString action;
    if (modifiedFields.contains(T_GUID))
        action = "createTag";
    else
        action = "updateTag";

    TBinaryProtocol *bin = new TBinaryProtocol();
    bin->writeMessageBegin(action, T_CALL, 0);
    bin->writeString(EdamProtocol::GetInstance()->getAuthenticationToken(), 1);
    bin->writeStructBegin(2);

    if (!modifiedFields.contains(T_GUID))
        bin->writeString(guid, 1);
    bin->writeString(name, 2);
    if (!parentGuid.isEmpty())
        bin->writeString(parentGuid, 3);

    bin->writeFieldStop();
    bin->writeFieldStop();
    QByteArray result = bin->getData();

    delete bin;
    return result;
}

void Tag::sync() {
    qDebug() << "Tag::sync()";

    bool ok;
    QByteArray result = EdamProtocol::GetInstance()->getNetworkManager()->postData(EdamProtocol::GetInstance()->getNoteStoreUri(), createPushContentPost(), ok);

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

    if (!data.contains(0)) {
        qDebug() << "KLAIDA!" << data.keys();
    }

    if (name.toLower() == "createtag") {
        QString oldGuid = guid;
        loadFromData(data[0].value<hash>());

        QSqlQuery query;
        query.prepare("UPDATE notesTags SET guid=:guid WHERE guid=:oldGuid");
        query.bindValue(":guid", guid);
        query.bindValue(":oldGuid", oldGuid);
        query.exec();
        query.clear();

        query.prepare("UPDATE syncStatus SET value=:guid WHERE option=:option AND value=:oldGuid");
        query.bindValue(":guid", guid);
        query.bindValue(":oldGuid", oldGuid);
        query.bindValue(":option", "selTag");
        query.exec();
        query.clear();

        query.prepare("DELETE FROM tags WHERE guid=:oldGuid");
        query.bindValue(":oldGuid", oldGuid);
        query.exec();
    }
    else
        updateSequenceNum = data[0].toInt();

    toSQL();

    if (updateSequenceNum > 0)
        EdamProtocol::GetInstance()->getSyncEngine()->updateUSN(updateSequenceNum);

    QSqlQuery query;
    query.prepare("DELETE FROM tagsUpdates WHERE guid=:guid");
    query.bindValue(":guid", guid);
    query.exec();
}

QString Tag::createNewTag(QString tagName) {
    guid = newGUID("tags", "guid");
    name = tagName;
    parentGuid = "";
    updateSequenceNum = 0;

    toSQL();
    editField(T_GUID);
    return guid;
}

void Tag::expungeTag(QString id) {
    QSqlQuery query;
    query.prepare("DELETE FROM tagsUpdates WHERE guid=:guid");
    query.bindValue(":guid", id);
    query.exec();
    query.clear();

    query.prepare("DELETE FROM tags WHERE guid=:guid");
    query.bindValue(":guid", id);
    query.exec();
    query.clear();

    query.prepare("DELETE FROM notesTags WHERE guid=:guid");
    query.bindValue(":guid", id);
    query.exec();
    query.clear();
}
