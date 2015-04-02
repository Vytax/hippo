#include "resource.h"
#include "sql.h"
#include "freedesktopmime.h"
#include "edamprotocol.h"
#include "Logger.h"
#include "error.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QImageReader>
#include <QFile>
#include <QDir>

Resource::Resource(QObject *parent) :
    QObject(parent)
{
    isNew = false;
    attachment = false;
    updateSequenceNum = 0;
}

Resource::Resource(QObject *parent, hash data):
        QObject(parent)
{
    if (data.contains(1))
        guid = QString::fromUtf8(data[1].toByteArray());

    if (data.contains(2))
        noteGuid = QString::fromUtf8(data[2].toByteArray());

    if (data.contains(3)) {
        hash d = data[3].value<hash>();

        if (d.contains(1))
            bodyHash = d[1].toByteArray().toHex();

        if (d.contains(2))
            size = d[2].toInt();
    }

    if (data.contains(4))
        mime = QString::fromUtf8(data[4].toByteArray());

    if (data.contains(5))
        width = data[5].toInt();

    if (data.contains(6))
        height = data[6].toInt();

    if (data.contains(11)) {
        hash d = data[11].value<hash>();

        if (d.contains(1))
            sourceURL = QString::fromUtf8(d[1].toByteArray());

        if (d.contains(10))
            fileName = QString::fromUtf8(d[10].toByteArray());

        if (d.contains(11))
            attachment = d[11].toBool();
    }

    if (data.contains(12))
        updateSequenceNum = data[12].toInt();

    isNew = false;

    toSQL();
}

Resource* Resource::fromGUID(QString id) {
    Resource *r = new Resource();
    if (r->loadGuid(id))
        return r;

    delete r;
    return NULL;
}

Resource* Resource::fromHash(QString hash) {
    Resource *r = new Resource();
    if (r->loadHash(hash))
        return r;

    delete r;
    return NULL;
}

bool Resource::loadGuid(QString id) {
    QSqlQuery query;
    query.prepare("SELECT noteGuid, bodyHash, width, height, new, updateSequenceNum, size, mime, fileName, sourceURL, attachment FROM resources WHERE guid=:guid");
    query.bindValue(":guid", id);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());

    if (!query.next())
         return false;

    guid = id;
    noteGuid = query.value(0).toString();
    bodyHash = query.value(1).toString();
    width = query.value(2).toInt();
    height = query.value(3).toInt();
    isNew = query.value(4).toBool();
    updateSequenceNum = query.value(5).toInt();
    size = query.value(6).toInt();
    mime = query.value(7).toString();
    fileName = query.value(8).toString();
    sourceURL = query.value(9).toString();
    attachment = query.value(10).toBool();

    return true;
}

bool Resource::loadHash(QString hash) {
    QSqlQuery query;
    query.prepare("SELECT noteGuid, guid, width, height, new, updateSequenceNum, size, mime, fileName, sourceURL, attachment FROM resources WHERE bodyHash=:bodyHash");
    query.bindValue(":bodyHash", hash);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());

    if (!query.next())
         return false;

    bodyHash = hash;
    noteGuid = query.value(0).toString();
    guid = query.value(1).toString();
    width = query.value(2).toInt();
    height = query.value(3).toInt();
    isNew = query.value(4).toBool();
    updateSequenceNum = query.value(5).toInt();
    size = query.value(6).toInt();
    mime = query.value(7).toString();
    fileName = query.value(8).toString();
    sourceURL = query.value(9).toString();
    attachment = query.value(10).toBool();

    return true;
}

void Resource::setNoteGuid(QString note)
{
    noteGuid = note;
}

QByteArray Resource::createGetContentPost()
{
    TBinaryProtocol *bin = new TBinaryProtocol();
    bin->writeMessageBegin("getResourceData", T_CALL, 0);
    bin->writeString(EdamProtocol::GetInstance()->getAuthenticationToken(), 1);
    bin->writeString(guid, 2);
    bin->writeFieldStop();
    QByteArray result = bin->getData();
    delete bin;
    return result;
}

void Resource::getContent(){
    LOG_INFO(QString("Fetching Resource Data (%1)").arg(guid));

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

    hash d = bin->readField();

    if (!d.contains(0)) {
        Error::readExceptions(d);
        return;
    }

    data = d[0].toByteArray();

    if (data.isEmpty())
        return;

    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    bodyHash = hash.toHex();

    toSQLData();

    delete bin;
}

void Resource::toSQL()
{
    QSqlQuery query;

    QString q("REPLACE INTO resources (guid, noteGuid, bodyHash, width, height, new, updateSequenceNum, size, mime, fileName, sourceURL, attachment)");
    q+= "VALUES (:guid, :noteGuid, :bodyHash, :width, :height, :new, :updateSequenceNum, :size, :mime, :fileName, :sourceURL, :attachment)";

    query.prepare(q);
    query.bindValue(":guid", guid);
    query.bindValue(":noteGuid", noteGuid);
    query.bindValue(":bodyHash", bodyHash);
    query.bindValue(":width", width);
    query.bindValue(":height", height);
    query.bindValue(":new", isNew);
    query.bindValue(":updateSequenceNum", updateSequenceNum);
    query.bindValue(":fileName", fileName);
    query.bindValue(":size", size);
    query.bindValue(":mime", mime);
    query.bindValue(":sourceURL", sourceURL);
    query.bindValue(":attachment", attachment);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());

    if (updateSequenceNum > 0)
        EdamProtocol::GetInstance()->getSyncEngine()->updateUSN(updateSequenceNum);
}

void Resource::toSQLData()
{
    QSqlQuery query;

    QString q("REPLACE INTO resourcesData (hash, data) ");
    q+= "VALUES (:hash, :data)";

    query.prepare(q);
    query.bindValue(":hash", bodyHash);
    query.bindValue(":data", data);

    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());
}

bool Resource::hasData()
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM resourcesData WHERE hash=:hash");
    query.bindValue(":hash", bodyHash);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());

    if (query.next())
        return query.value(0).toInt() > 0;

    return false;
}

QByteArray Resource::getData() {
    QSqlQuery query;
    query.prepare("SELECT data FROM resourcesData WHERE hash=:hash");
    query.bindValue(":hash", bodyHash);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());

    if (query.next())
        return query.value(0).toByteArray();

    return QByteArray();
}

bool Resource::create(QString file)
{    
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    if (!file.startsWith(QDir::tempPath(), Qt::CaseInsensitive)) {
        QFileInfo info(f);
        fileName = info.fileName();
        LOG_INFO(QString("New resource from file (%1)").arg(fileName));
    }

    size = f.size();

    if (size > 15000000) {
        f.close();
        return false;
    } else {
        LOG_ERROR("Resource is too large.");
    }

    data = f.readAll();
    f.close();

    QFreeDesktopMime m;
    mime = m.fromFile(file);

    if (isImage()) {
        QImageReader img(file);

        if (!img.canRead())
            return false;

        width = img.size().width();
        height = img.size().height();
    }
    guid = newGUID("resources", "guid");

    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    bodyHash = hash.toHex();

    isNew = true;

    toSQL();
    if (!hasData())
        toSQLData();

    Note::editField(noteGuid, Note::T_RECOURCES);

    return true;
}

 bool Resource::isImage()
 {
     QStringList imageTypes;
     imageTypes << "image/gif" << "image/jpeg" << "image/png";

     return imageTypes.contains(mime);
 }

 bool Resource::isPDF()
 {
     return mime.toLower() == "application/pdf";
 }

 QString Resource::getBodyHash()
 {
     return bodyHash;
 }

 QString Resource::mimeType()
 {
     return mime;
 }

 QString Resource::getFileName()
 {
     return fileName;
 }
