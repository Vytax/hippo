#include "listitem.h"

ListItem::ListItem(QListWidget *view):
    QListWidgetItem(view, QListWidgetItem::UserType)
{   
    sortType = byTitle;
}

void ListItem::setGUID(QString id)
{
    guid = id;
}

QString ListItem::getGUID()
{
    return guid;
}

void ListItem::setNoteBookGUID(QString id)
{
    noteBookGuid = id;
}

void ListItem::setActive(bool value)
{
    active = value;
}

QByteArray ListItem::encode()
{
    QString result = guid + "@" + noteBookGuid + "@";

    if (active)
        result+= "active";

    return result.toLatin1();
}

bool ListItem::isActive()
{
    return active;
}

bool ListItem::operator< ( const QListWidgetItem & other ) const
{
    ListItem &o = (ListItem &)other;
    if (sortType == byTitle) {
        return text() < o.text();
    } else if (sortType == byCreated) {
        return created < o.getCreated();
    } else if (sortType == byModified) {
        return modified < o.getModified();
    }
    return true;
}

void ListItem::setModified(qint64 value)
{
    modified = value;
}

qint64 ListItem::getModified()
{
    return modified;
}

void ListItem::setCreated(qint64 value)
{
    created = value;
}

qint64 ListItem::getCreated()
{
    return created;
}

void ListItem::setSortType(qint8 type)
{
    sortType = type;
}
