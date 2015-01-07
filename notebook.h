#ifndef NOTEBOOK_H
#define NOTEBOOK_H

#include "tbinaryprotocol.h"
#include <QObject>
#include <QDateTime>


class NoteBook : public QObject
{
    Q_OBJECT
public:
    NoteBook();
    void loadFromData(hash data);
    bool loadFromSQL(QString id);
    QString getGUID();
    void toSQL();

    enum NoteBookField {
        T_NEW = 0,
        T_GUID = 1,
        T_NAME = 2,
        T_DEFAULT = 3,
        T_CREATED = 4,
        T_UPDATED = 5,
        T_STACK = 6
    };

    void editField(int field);
    static void editField(QString id, int field);

    typedef QHash<NoteBookField, QVariant> NoteBookUpdates;
    void update(NoteBookUpdates updates);
    void sync();

    QString createNewNotebook(QString notebookName);
    static void expungeNotebook(QString id);

private:
    QString guid;
    QString name;
    qint32 updateSequenceNum;
    bool defaultNotebook;
    QDateTime serviceCreated;
    QDateTime serviceUpdated;
    QString stack;

    QByteArray createPushContentPost();

};

#endif // NOTEBOOK_H
