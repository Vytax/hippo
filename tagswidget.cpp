#include "tagswidget.h"
#include "sql.h"
#include "renamedialog.h"
#include "tag.h"
#include "note.h"

#include <QSqlQuery>
#include <QAction>
#include <QMenu>
#include <QDebug>
#include <QTextCodec>
#include <QMimeData>
#include <QDrag>

TagsWidget::TagsWidget(QWidget *parent) :
    QTreeWidget(parent), signalsDisabled(false), itemsFlag(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled)
{
    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDragDropOverwriteMode(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
}

void TagsWidget::selectTagWithGUID(QString name)
{
    QTreeWidgetItemIterator it(this);
    while (*it) {
        TreeItem* item = reinterpret_cast<TreeItem*>(*it);
        if (item->getGUIDs().first() == name) {
            setCurrentItem(item);
            return;
        }
        ++it;
    }
}

void TagsWidget::reload()
{
    clear();
    signalsDisabled = true;

    QSqlQuery result;

    QHash<QString, TreeItem*> tags;

    result.exec("SELECT name, guid, parentGuid FROM tags");

    while (result.next()) {
        TreeItem* item = new TreeItem();
        QString id = result.value(1).toString();
        item->appendGUID(id);
        item->setText(result.value(0).toString());
        item->setParentGuid(result.value(2).toString());
        item->setFlags(itemsFlag);
        item->setIcon(0, QIcon("://img/tag.png"));
        tags[id] = item;
    }

    QString id;
    foreach( id, tags.keys()) {
        TreeItem* item = tags[id];
        if (item->getParentGuid().isEmpty())
            addTopLevelItem(item);
        else {
            if (tags.contains(item->getParentGuid()))
                    tags[item->getParentGuid()]->addChild(item);
        }

        QString parent = item->getParentGuid();
        while(!parent.isEmpty()) {
            tags[parent]->appendGUID(item->getGUIDs().first());
            parent = tags[parent]->getParentGuid();
        }
    }

    expandAll();
    updateCounts();
    signalsDisabled = false;
}

int TagsWidget::notesCountWithTag(QStringList id)
{
    if (id.isEmpty())
        return 0;

    QSqlQuery count;
    count.prepare(QString("SELECT COUNT(notesTags.noteGuid) FROM notesTags LEFT JOIN notes ON notesTags.noteGuid = notes.guid WHERE notesTags.guid IN (%1) AND notes.active=:active").arg(AddSQLArray(id.count())));
    count.bindValue(":active", true);
    BindSQLArray(count, id);

    count.exec();

    if (count.next())
        return count.value(0).toInt();

    return 0;
}

void TagsWidget::updateCounts() {

    QTreeWidgetItemIterator it(this);
    while (*it) {
        TreeItem* item = reinterpret_cast<TreeItem*>(*it);

        item->setCount(notesCountWithTag(item->getGUIDs()));

        ++it;
    }
}

QStringList TagsWidget::currentGuids() {
    if (currentItem() != NULL) {
        TreeItem* n = reinterpret_cast<TreeItem*>(currentItem());
        return n->getGUIDs();
    }
    return QStringList();
}

void TagsWidget::contextMenuEvent (QContextMenuEvent * event)
{
    qDebug() << "contextMenuEvent";
    TreeItem* item = reinterpret_cast<TreeItem*>(this->itemAt(event->pos()));

    if (item == NULL) {
        event->ignore();
        return;
    }

    QMenu menu(this);

    QAction *rename = new QAction(QIcon::fromTheme("edit-rename"), "Rename...", this);
    rename->setObjectName("rename");
    menu.addAction(rename);

    QAction *removeAllA = new QAction("Remove From All Notes...", this);
    removeAllA->setObjectName("removeAll");
    removeAllA->setDisabled(item->getCount() == 0);
    menu.addAction(removeAllA);

    QAction *ret = menu.exec(event->globalPos());

    if (ret == NULL) {
        event->ignore();
        return;
    }

    if (ret->objectName() == "rename")
        renameTag(item);
    else if (ret->objectName() == "removeAll")
        removeAll(item);
}

void TagsWidget::renameTag(TreeItem *item) {
    QString guid = item->getGUIDs().first();

    RenameDialog dialog(this);
    dialog.setWindowTitle("Rename Tag");
    dialog.setLabel("Tag name");
    dialog.setText(item->getText());

    if (dialog.exec() != QDialog::Accepted)
        return;

    QString newTagName = dialog.getText();
    item->setText(newTagName);

    Tag *tag = new Tag();
    if (tag->loadFromSQL(guid)) {
        Tag::TagUpdates updates;
        updates[Tag::T_NAME] = newTagName;
        tag->update(updates);
    }
    delete tag;
    emit tagsUpdated();
}

void TagsWidget::removeAll(TreeItem *item) {
    QString guid = item->getGUIDs().first();

    QSqlQuery query;
    query.prepare("SELECT noteGuid FROM notesTags WHERE guid=:guid");
    query.bindValue(":guid", guid);
    query.exec();

    QStringList notes;

    while (query.next())
        notes.append(query.value(0).toString());

    query.prepare("DELETE FROM notesTags WHERE guid=:guid");
    query.bindValue(":guid", guid);
    query.exec();

    QString note;

    foreach(note, notes) {
        Note::editField(note, Note::T_TAG_GUIDS);
    }

    updateCounts();
    emit tagsUpdated();
}

Qt::DropActions TagsWidget::supportedDropActions () const
{
    return Qt::MoveAction;
}

void TagsWidget::dragMoveEvent(QDragMoveEvent *event)
{
    TreeItem* item = reinterpret_cast<TreeItem*>(itemAt(event->pos()));

    if (event->mimeData()->hasFormat("qevernote/note")) {
        if (item == NULL) {
            event->ignore();
            return;
        }

        QString data = QString::fromLatin1(event->mimeData()->data("qevernote/note"));

        QStringList parts = data.split('@');
        if (parts[2] == "active") {
            event->setDropAction(Qt::MoveAction);
            event->accept();
            return;
        }

        event->ignore();
        return;
    }

    if (event->mimeData()->hasFormat("qevernote/tag")) {
        if (item == NULL) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
            return;
        }

        QString data = QString::fromLatin1(event->mimeData()->data("qevernote/tag"));
        if (item->getGUIDs().contains(data)) {
            event->ignore();
            return;
        }

        event->setDropAction(Qt::MoveAction);
        event->accept();
        return;
    }

    event->ignore();
}

void TagsWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("qevernote/note") || event->mimeData()->hasFormat("qevernote/tag"))
        event->accept();
    else
        event->ignore();
}

void TagsWidget::dropEvent(QDropEvent * event)
{
    if (event->mimeData()->hasFormat("qevernote/note")) {

        QString data = QString::fromLatin1(event->mimeData()->data("qevernote/note"));

        QStringList parts = data.split('@');
        QString noteGuid = parts[0];
        bool active = (parts[2] == "active");

        TreeItem* item = reinterpret_cast<TreeItem*>(itemAt(event->pos()));

        if (item != NULL && active) {
            emit tagAdded(noteGuid, item->getGUIDs().first());
            event->accept();
        }
    }
    else if (event->mimeData()->hasFormat("qevernote/tag")) {
        QString data = QString::fromLatin1(event->mimeData()->data("qevernote/tag"));

        TreeItem* item = reinterpret_cast<TreeItem*>(itemAt(event->pos()));
        QString parentGuid;

        if (item != NULL)
            parentGuid = item->getGUIDs().first();

        Tag *tag = new Tag();
        if (tag->loadFromSQL(data)) {
            Tag::TagUpdates updates;
            updates[Tag::T_PARENT] = parentGuid;
            tag->update(updates);
        }
        delete tag;
        reload();
    }

    event->ignore();
}

void TagsWidget::startDrag( Qt::DropActions supportedActions )
{
    TreeItem* item = reinterpret_cast<TreeItem*>(currentItem());

    if (item == NULL)
        return;

    QMimeData *data = new QMimeData;
    data->setData("qevernote/tag", item->getGUIDs().first().toLatin1());

    QDrag *drag = new QDrag(this);
    drag->setPixmap(QIcon("://img/tag_add.png").pixmap(24,24));
    drag->setHotSpot(QPoint(12,12));
    drag->setMimeData(data);

    drag->exec(supportedActions, Qt::MoveAction);
}
