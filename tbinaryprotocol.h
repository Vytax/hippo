#ifndef TBINARYPROTOCOL_H
#define TBINARYPROTOCOL_H

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QHash>
#include <QVariant>
#include <QList>

typedef QHash<qint16, QVariant> hash;
typedef QList<QVariant> list;

Q_DECLARE_METATYPE(hash)

enum TMessageType {
  T_CALL       = 1,
  T_REPLY      = 2,
  T_EXCEPTION  = 3,
  T_ONEWAY     = 4
};

static const qint32 VERSION_1 = 0x80010000;
static const qint32 VERSION_MASK = 0xffff0000;

class TBinaryProtocol
{
public:
    enum TType {
      T_STOP       = 0,
      T_VOID       = 1,
      T_BOOL       = 2,
      T_BYTE       = 3,
      T_I08        = 3,
      T_I16        = 6,
      T_I32        = 8,
      T_U64        = 9,
      T_I64        = 10,
      T_DOUBLE     = 4,
      T_STRING     = 11,
      T_UTF7       = 11,
      T_STRUCT     = 12,
      T_MAP        = 13,
      T_SET        = 14,
      T_LIST       = 15,
      T_UTF8       = 16,
      T_UTF16      = 17
    };

    TBinaryProtocol();
    TBinaryProtocol(QByteArray bin);
    ~TBinaryProtocol();

    void writeMessageBegin(QString name, TMessageType messageType, qint32 seqid);
    void writeString(QString str, qint16 fieldId = -1);
    void writeBinary(QByteArray bin, qint16 fieldId);
    void writeI16(qint16 value, qint16 fieldId);
    void writeI32(qint32 value, qint16 fieldId);
    void writeI64(qint64 value, qint16 fieldId);
    void writeBool(bool value, qint16 fieldId);
    void writeFieldStop();    
    void writeStructBegin(qint16 fieldId);
    void writeListBegin(qint16 fieldId, TType fieldType, qint32 size);

    void readMessageBegin(QString &name, TMessageType &messageType, qint32 &seqid);    
    hash readField();

    QByteArray getData();
    void save();
private:
    QIODevice::OpenModeFlag openMode;

    QByteArray *data;
    QDataStream *stream;

    void writeFieldBegin(TType fieldType, qint16 fieldId);
    QByteArray readByteArray();
    void readFieldBegin(TType &fieldType, qint16 &fieldId);
    bool readBool();
    qint64 readI64();
    qint32 readI32();
    qint16 readI16();
    double readDouble();
    list readList();
    QVariant readVariant(TType fieldType);
    QMap<QString, QVariant> readMap();
};

#endif // TBINARYPROTOCOL_H
