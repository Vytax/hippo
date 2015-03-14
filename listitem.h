#ifndef LISTITEM_H
#define LISTITEM_H

#include <QListWidgetItem>

enum noteSortType {
    byTitle = 1,
    byModified = 2,
    byCreated = 3,
    byTitleDesc = 4,
    byModifiedDesc = 5,
    byCreatedDesc = 6
};

class ListItem : public QListWidgetItem
{
public:
    ListItem(QListWidget *view = 0);

    void setGUID(QString id);
    QString getGUID();

    void setNoteBookGUID(QString id);

    void setActive(bool value);
    QByteArray encode();
    bool isActive();
    void setModified(qint64 value);
    qint64 getModified();
    void setCreated(qint64 value);
    qint64 getCreated();

    void setSortType(qint8 type);

    bool operator< ( const QListWidgetItem & other ) const;

public slots:
    void changeGuid(QString from, QString to);

private:
    QString guid;
    QString noteBookGuid;
    bool active;

    qint8 sortType;
    qint64 created;
    qint64 modified;
};

#endif // LISTITEM_H
