#include "tbinaryprotocol.h"
#include "Logger.h"
#include <QFile>

TBinaryProtocol::TBinaryProtocol()
{
    openMode = QIODevice::WriteOnly;

    data = new QByteArray();
    stream = new QDataStream(data, openMode);
    stream->setByteOrder(QDataStream::BigEndian);
}

TBinaryProtocol::TBinaryProtocol(QByteArray bin)
{
    openMode = QIODevice::ReadOnly;

    data = new QByteArray(bin);
    stream = new QDataStream(data, openMode);
    stream->setByteOrder(QDataStream::BigEndian);
}

TBinaryProtocol::~TBinaryProtocol()
{
    delete data;
    delete stream;
}

void TBinaryProtocol::writeMessageBegin(QString name, TMessageType messageType, qint32 seqid)
{
    qint32 version = (VERSION_1) | ((qint32)messageType);
    *stream << version;
    writeString(name, -1);
    *stream << seqid;
}

void TBinaryProtocol::writeString(QString str, qint16 fieldId)
{
    writeBinary(str.toUtf8(), fieldId);
}

void TBinaryProtocol::writeBinary(QByteArray bin, qint16 fieldId)
{
    if (fieldId >= 0)
        writeFieldBegin(T_STRING, fieldId);

    quint32 size = bin.size();

    *stream << size;
    if (size > 0) {
        stream->writeRawData(bin.data(), size);
    }
}

void TBinaryProtocol::writeFieldBegin(TType fieldType, qint16 fieldId)
{
    *stream << (qint8)fieldType;
    *stream << fieldId;
}

void TBinaryProtocol::writeFieldStop()
{
    *stream << (qint8)T_STOP;
}

void TBinaryProtocol::writeStructBegin(qint16 fieldId)
{
     writeFieldBegin(T_STRUCT, fieldId);
}

void TBinaryProtocol::writeListBegin(qint16 fieldId, TType fieldType, qint32 size)
{
    writeFieldBegin(T_LIST, fieldId);
    *stream << (qint8)fieldType;
    *stream << size;
}

void TBinaryProtocol::writeI16(qint16 value, qint16 fieldId)
{
    writeFieldBegin(T_I16, fieldId);
    *stream << value;
}

void TBinaryProtocol::writeI32(qint32 value, qint16 fieldId)
{
    writeFieldBegin(T_I32, fieldId);
    *stream << value;
}

void TBinaryProtocol::writeI64(qint64 value, qint16 fieldId)
{
    writeFieldBegin(T_I64, fieldId);
    *stream << value;
}

void TBinaryProtocol::writeBool(bool value, qint16 fieldId)
{
    writeFieldBegin(T_BOOL, fieldId);
    quint8 tmp =  value ? 1 : 0;
    *stream << tmp;
}

void TBinaryProtocol::readMessageBegin(QString &name, TMessageType &messageType, qint32 &seqid)
{
    qint32 sz;
    *stream >> sz;

    if (sz < 0) {
        qint32 version = sz & VERSION_MASK;
        if (version != VERSION_1) {
            LOG_ERROR("Bad version identifier");
            return;
        }
        messageType = (TMessageType)(sz & 0x000000ff);
        name = QString::fromUtf8(readByteArray());
        *stream >> seqid;
    } else {

        char * s = new char[sz];
        stream->readRawData(s, sz);
        name = QString::fromUtf8(s,sz);
        delete s;
        qint8 type;
        *stream >> type;
        messageType = (TMessageType)type;
        *stream >> seqid;
    }
}

QByteArray TBinaryProtocol::readByteArray()
{
    qint32 size;
    *stream >> size;

    char * s = new char[size];
    stream->readRawData(s, size);
    QByteArray result = QByteArray(s, size);
    delete s;

    return result;
}

void TBinaryProtocol::readFieldBegin(TType &fieldType, qint16 &fieldId)
{
    qint8 type;
    *stream >> type;
    fieldType = (TType)type;
    if (fieldType == T_STOP)
        fieldId = 0;
    else
        *stream >> fieldId;
}

bool TBinaryProtocol::readBool()
{
    qint8 b;
    *stream >> b;

    return b != 0;
}

qint64 TBinaryProtocol::readI64()
{
    qint64 i;
    *stream >> i;
    return i;
}

qint32 TBinaryProtocol::readI32()
{
    qint32 i;
    *stream >> i;
    return i;
}

qint16 TBinaryProtocol::readI16()
{
    qint16 i;
    *stream >> i;
    return i;
}

double TBinaryProtocol::readDouble()
{
    double i;
    *stream >> i;
    return i;
}

list TBinaryProtocol::readList()
{
    qint8 ftype;
    *stream >> ftype;

    qint32 size = readI32();    

    list result;

    for (int i=0; i<size; i++){
        result.append(readVariant((TType)ftype));
    }
    return result;
}

QVariant TBinaryProtocol::readVariant(TType fieldType)
{
    if (fieldType == T_STRUCT) {
        QVariant var;
        var.setValue(readField());
        return var;
    }
    else if (fieldType == T_I64) {
        return QVariant(readI64());
    }
    else if (fieldType == T_STRING) {
        return QVariant(readByteArray());
    }
    else if (fieldType == T_I32) {
        return QVariant(readI32());
    }
    else if (fieldType == T_I16) {
        return QVariant(readI16());
    }
    else if (fieldType == T_BOOL) {
        return QVariant(readBool());
    }
    else if (fieldType == T_LIST) {
        return QVariant(readList());
    }
    else if (fieldType == T_DOUBLE) {
        return QVariant(readDouble());
    }
    else if (fieldType == T_SET) {
        return QVariant(readList());
    }
    else if (fieldType == T_MAP) {
        return QVariant(readMap());
    }
    else {
        LOG_ERROR(QString("Unknown Tag: %1").arg(fieldType));
        return QVariant();
    }
}

hash TBinaryProtocol::readField()
{    
    hash result;
    while(true){
        TType ftype;
        qint16 fieldId;
        readFieldBegin(ftype, fieldId);

        if (ftype == T_STOP)
            break;

        QVariant value = readVariant(ftype);
        if (value.isValid())
            result[fieldId] = value;
        else
            break;
    }    
    return result;
}

QMap<QString, QVariant> TBinaryProtocol::readMap() {
    QMap<QString, QVariant> result;

    qint8 ktype, vtype;
    *stream >> ktype;
    *stream >> vtype;

    qint32 size;
    *stream >> size;

    for (int i=0; i<size; i++) {
        QVariant key = readVariant((TType)ktype);
        QVariant val = readVariant((TType)vtype);
        result[key.toString()] = val;
    }

    return result;
}

QByteArray TBinaryProtocol::getData()
{
    return *data;
}

void TBinaryProtocol::save()
{
    QFile file("test.bin");
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);
    out.writeRawData(data->data(), data->size());
    file.close();
}
