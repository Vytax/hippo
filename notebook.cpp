#include "notebook.h"
#include "edamprotocol.h"
#include <QSqlQuery>

#include <QDebug>


NoteBook::NoteBook()
{
    updateSequenceNum = 0;
}

void NoteBook::loadFromData(hash data)
{
    qDebug() << "readData" << data.keys();
    if (data.contains(1))
        guid = QString::fromUtf8(data[1].toByteArray());
    if (data.contains(2))
        name = QString::fromUtf8(data[2].toByteArray());
    if (data.contains(5))
        updateSequenceNum = data[5].toInt();
    if (data.contains(6))
        defaultNotebook = data[6].toBool();
    if (data.contains(7))
        serviceCreated = QDateTime::fromMSecsSinceEpoch(data[7].toLongLong());
    if (data.contains(8))
        serviceUpdated = QDateTime::fromMSecsSinceEpoch(data[8].toLongLong());
    if (data.contains(12))
        stack = QString::fromUtf8(data[12].toByteArray());
}

bool NoteBook::loadFromSQL(QString id) {
    QSqlQuery query;
    query.prepare("SELECT name, defaultNotebook, serviceCreated, serviceUpdated, stack FROM notebooks WHERE guid=:guid");
    query.bindValue(":guid", id);
    query.exec();

    if (!query.next())
         return false;

    guid = id;
    name = query.value(0).toString();
    defaultNotebook = query.value(1).toBool();
    serviceCreated = QDateTime::fromMSecsSinceEpoch(query.value(2).toLongLong());
    serviceUpdated = QDateTime::fromMSecsSinceEpoch(query.value(3).toLongLong());
    stack = query.value(4).toString();

    return true;
}

QString NoteBook::getGUID()
{
    return guid;
}

void NoteBook::toSQL()
{
    QString q("REPLACE INTO notebooks (guid, name, updateSequenceNum, defaultNotebook, serviceCreated, serviceUpdated, stack)");
    q+= "VALUES (:guid, :name, :updateSequenceNum, :defaultNotebook, :serviceCreated, :serviceUpdated, :stack)";
    QSqlQuery query;
    query.prepare(q);
    query.bindValue(":guid", guid);
    query.bindValue(":name", name);
    query.bindValue(":updateSequenceNum", updateSequenceNum);
    query.bindValue(":defaultNotebook", defaultNotebook);
    query.bindValue(":serviceCreated", serviceCreated.toMSecsSinceEpoch());
    query.bindValue(":serviceUpdated", serviceUpdated.toMSecsSinceEpoch());
    query.bindValue(":stack", stack);
    query.exec();

    if (updateSequenceNum > 0)
        EdamProtocol::GetInstance()->getSyncEngine()->updateUSN(updateSequenceNum);
}

void NoteBook::editField(int field) {
    editField(guid, field);
}

void NoteBook::editField(QString id, int field) {
    if (id.isEmpty())
        return;

    QSqlQuery query;
    query.prepare("INSERT INTO notebookUpdates (guid, field) VALUES (:guid, :field)");
    query.bindValue(":guid", id);
    query.bindValue(":field", field);
    query.exec();
}

void NoteBook::update(NoteBookUpdates updates) {
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
    if (updates.contains(T_DEFAULT)) {
        defaultNotebook = updates[T_DEFAULT].toBool();
        editField(T_DEFAULT);
    }
    if (updates.contains(T_CREATED)) {
        serviceCreated = updates[T_CREATED].toDateTime();
        editField(T_CREATED);
    }
    if (updates.contains(T_UPDATED)) {
        serviceUpdated = updates[T_UPDATED].toDateTime();
        editField(T_UPDATED);
    }
    if (updates.contains(T_STACK)) {
        stack = updates[T_STACK].toString();
        editField(T_STACK);
    }
    toSQL();
}

QByteArray NoteBook::createPushContentPost() {
    QSqlQuery query;
    query.prepare("SELECT field FROM notebookUpdates WHERE guid=:guid");
    query.bindValue(":guid", guid);
    query.exec();

    QList<int> modifiedFields;

    while (query.next())
        modifiedFields.append(query.value(0).toInt());

    QString action;
    if (modifiedFields.contains(T_GUID))
        action = "createNotebook";
    else
        action = "updateNotebook";

    TBinaryProtocol *bin = new TBinaryProtocol();
    bin->writeMessageBegin(action, T_CALL, 0);
    bin->writeString(EdamProtocol::GetInstance()->getAuthenticationToken(), 1);
    bin->writeStructBegin(2);

    if (!modifiedFields.contains(T_GUID))
        bin->writeString(guid, 1);

    bin->writeString(name, 2);
    if (modifiedFields.contains(T_DEFAULT))
        bin->writeBool(defaultNotebook, 6);
    if (modifiedFields.contains(T_CREATED))
        bin->writeI64(serviceCreated.toMSecsSinceEpoch(), 7);
    if (modifiedFields.contains(T_UPDATED))
        bin->writeI64(serviceUpdated.toMSecsSinceEpoch(), 8);
    if (!stack.isEmpty())
        bin->writeString(stack, 12);


    bin->writeFieldStop();
    bin->writeFieldStop();
    QByteArray result = bin->getData();

    delete bin;
    return result;
}

void NoteBook::sync() {
    qDebug() << "NoteBook::sync()";

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
        qDebug() << "Error:" << "checkVersion failed: unknown result";
        return;
    }
    qDebug() << name << messageType << seqid;

    hash data = bin->readField();
    delete bin;

    if (!data.contains(0)) {
        qDebug() << "KLAIDA!" << data.keys();
    }

    if (name.toLower() == "createnotebook") {
        QString oldGuid = guid;
        loadFromData(data[0].value<hash>());

        QSqlQuery query;
        query.prepare("UPDATE notes SET notebookGuid=:notebookGuid WHERE notebookGuid=:oldGuid");
        query.bindValue(":notebookGuid", guid);
        query.bindValue(":oldGuid", oldGuid);
        query.exec();
        query.clear();

        query.prepare("UPDATE syncStatus SET value=:guid WHERE option=:option AND value=:oldGuid");
        query.bindValue(":guid", QString("notebook@%1").arg(guid));
        query.bindValue(":oldGuid", QString("notebook@%1").arg(oldGuid));
        query.bindValue(":option", "selNotebook");
        query.exec();
        query.clear();

        query.prepare("DELETE FROM notebooks WHERE guid=:oldGuid");
        query.bindValue(":oldGuid", oldGuid);
        query.exec();
    }
    else
        updateSequenceNum = data[0].toInt();

    toSQL();

    if (updateSequenceNum > 0)
        EdamProtocol::GetInstance()->getSyncEngine()->updateUSN(updateSequenceNum);

    QSqlQuery query;
    query.prepare("DELETE FROM notebookUpdates WHERE guid=:guid");
    query.bindValue(":guid", guid);
    query.exec();
}

QString NoteBook::createNewNotebook(QString notebookName) {
    guid = newGUID("notebooks", "guid");
    name = notebookName;
    stack = "";
    serviceCreated = QDateTime::currentDateTime();
    serviceUpdated = QDateTime::currentDateTime();

    editField(T_GUID);
    editField(T_NAME);
    editField(T_STACK);
    editField(T_CREATED);
    editField(T_UPDATED);

    toSQL();

    return guid;
}

void NoteBook::expungeNotebook(QString id) {
    QSqlQuery query;
    query.prepare("DELETE FROM notebooks WHERE guid=:guid");
    query.bindValue(":guid", id);
    query.exec();
}
