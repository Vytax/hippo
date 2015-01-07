#include "noteswidget.h"
#include "sql.h"
#include <QMimeData>
#include <QMenu>
#include <QSqlQuery>

#include <QDebug>

NotesWidget::NotesWidget(QWidget *parent) :
    QListWidget(parent)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setDragDropOverwriteMode(true);
    setSortingEnabled(true);
    listType = TreeItem::allNotes;
    signalsDisabled = false;
    sortType = byTitle;

    QVariant sort = sql::readSyncStatus("noteSortType");
    if (!sort.isNull())
        sortType = sort.toInt();

    connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(switchNote()));
}

void NotesWidget::startDrag( Qt::DropActions supportedActions )
{
    ListItem* l = reinterpret_cast<ListItem*>(currentItem());

    if (l == NULL)
        return;

    QMimeData *data = new QMimeData;
    data->setData("qevernote/note", l->encode());

    QDrag *drag = new QDrag(this);
    drag->setPixmap(l->icon().pixmap(24, 24));
    drag->setHotSpot(QPoint(12,12));
    drag->setMimeData(data);

    drag->exec(supportedActions, Qt::MoveAction);
}

void NotesWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("qevernote/note"))
        event->acceptProposedAction();
}

void NotesWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("qevernote/note"))
        event->acceptProposedAction();
}

void NotesWidget::selectNoteWithGuid(QString guid)
{
    ListItem *item = getNoteWithGuid(guid);

    if (item == NULL)
        return;

    setCurrentItem(item);
}

ListItem* NotesWidget::getNoteWithGuid(QString guid)
{
    ListItem* item = reinterpret_cast<ListItem*>(this->currentItem());
    if ((item != NULL) && (item->getGUID() == guid))
            return item;

    for (int i = 0; i < count(); i++) {
        ListItem* item = reinterpret_cast<ListItem*>(this->item(i));
        if (item->getGUID() == guid) {
            return item;
        }
    }
    return NULL;
}

void NotesWidget::contextMenuEvent (QContextMenuEvent * event)
{
    qDebug() << "contextMenuEvent";
    ListItem* item = reinterpret_cast<ListItem*>(this->itemAt(event->pos()));

    QMenu menu(this);

    QMenu *sortMenu = menu.addMenu(QIcon::fromTheme("view-sort-ascending"), "Sort By");
    QActionGroup *sortActions = new QActionGroup(this);
    sortActions->setExclusive(true);

    QAction *sortByTitle = new QAction("Title", this);
    sortByTitle->setObjectName("sortByTitle");
    sortByTitle->setCheckable(true);
    sortByTitle->setChecked(sortType == byTitle);
    sortMenu->addAction(sortByTitle);
    sortActions->addAction(sortByTitle);

    QAction *sortByCreated = new QAction("Created Date", this);
    sortByCreated->setObjectName("sortByCreated");
    sortByCreated->setCheckable(true);
    sortByCreated->setChecked(sortType == byCreated);
    sortMenu->addAction(sortByCreated);
    sortActions->addAction(sortByCreated);

    QAction *sortByModified = new QAction("Modified Date", this);
    sortByModified->setObjectName("sortByModified");
    sortByModified->setCheckable(true);
    sortByModified->setChecked(sortType == byModified);
    sortMenu->addAction(sortByModified);
    sortActions->addAction(sortByModified);


    if (item == NULL) {
        QAction *newNote = new QAction(QIcon::fromTheme("document-new"), "New Note...", this);
        newNote->setObjectName("new");
        newNote->setDisabled(listType == TreeItem::trashBin);
        menu.addAction(newNote);
    } else {
        if (item->isActive()) {
            QAction *del = new QAction(QIcon::fromTheme("edit-delete"), "Delete Note", this);
            del->setObjectName("delete");
            menu.addAction(del);
        } else {
            QAction *res = new QAction("Restore", this);
            res->setObjectName("restore");
            menu.addAction(res);
        }
    }

    QAction *ret = menu.exec(event->globalPos());

    if (ret == NULL) {
        event->ignore();
        return;
    }

    if (ret->objectName() == "delete")
        emit noteDeleted(item->getGUID());
    else if (ret->objectName() == "restore")
        emit noteRestored(item->getGUID());
    else if (ret->objectName() == "new")
        emit noteCreated();
    else if (ret->objectName() == "sortByTitle")
        setNoteSortType(byTitle);
    else if (ret->objectName() == "sortByCreated")
        setNoteSortType(byCreated);
    else if (ret->objectName() == "sortByModified")
        setNoteSortType(byModified);

    qDebug() << "dd" << ret->objectName();

    event->accept();
}

void NotesWidget::switchNotebook(TreeItem::noteListType type, QStringList id, QString currentNote)
{
    listType = type;
    signalsDisabled = true;
    bool active = true;

    clear();

    QSqlQuery result;
    if (type == TreeItem::allNotes){
        result.prepare("SELECT title, guid, notebookGuid, created, updated FROM notes WHERE active=:active");
        result.bindValue(":active", true);
    } else if ((type == TreeItem::noteBook) || (type == TreeItem::stack)) {
        result.prepare(QString("SELECT title, guid, notebookGuid, created, updated FROM notes WHERE notebookGuid IN (%1) AND active=:active").arg(AddSQLArray(id.count())));
        BindSQLArray(result, id);
        result.bindValue(":active", true);
    } else if (type == TreeItem::trashBin) {
        result.prepare("SELECT title, guid, notebookGuid, created, updated FROM notes WHERE active=:active");
        result.bindValue(":active", false);
        active = false;
    } else if (type == TreeItem::conflict) {
        result.prepare("SELECT notes.title, conflictingNotes.guid, notes.notebookGuid, notes.created, conflictingNotes.updated FROM conflictingNotes LEFT JOIN notes ON notes.guid = conflictingNotes.guid");
    }
    result.exec();

    while (result.next()) {
        ListItem* item = new ListItem(this);
        item->setText(result.value(0).toString());
        item->setGUID(result.value(1).toString());
        item->setNoteBookGUID(result.value(2).toString());
        item->setCreated(result.value(3).toLongLong());
        item->setModified(result.value(4).toLongLong());
        item->setIcon(QIcon::fromTheme("basket"));
        item->setActive(active);
        item->setSortType(sortType);
    }

    if (!currentNote.isEmpty())
        selectNoteWithGuid(currentNote);

    sortItems();

    signalsDisabled = false;
    emit reloaded();
}

void NotesWidget::switchNote()
{
    if (signalsDisabled)
        return;

    emit noteSwitched();
}

void NotesWidget::switchTag(QStringList id, QString currentNote)
{
    signalsDisabled = true;
    clear();

    if (id.isEmpty()) {
        signalsDisabled = false;
        return;
    }

    QSqlQuery result;

    result.prepare(QString("SELECT notes.title, notes.guid, notes.created, notes.updated FROM notesTags LEFT OUTER JOIN notes ON notesTags.noteGuid = notes.guid WHERE notesTags.guid IN (%1) AND notes.active=:active GROUP BY notesTags.noteGuid").arg(AddSQLArray(id.count())));
    BindSQLArray(result, id);

    result.bindValue(":active", true);
    result.exec();

    while (result.next()) {
        ListItem* item = new ListItem(this);
        item->setText(result.value(0).toString());
        item->setGUID(result.value(1).toString());
        item->setCreated(result.value(2).toLongLong());
        item->setModified(result.value(3).toLongLong());
        item->setIcon(QIcon::fromTheme("basket"));
        item->setSortType(sortType);
    }

    if (!currentNote.isEmpty())
        selectNoteWithGuid(currentNote);

    sortItems();

    signalsDisabled = false;
    emit reloaded();
}

void NotesWidget::setNoteSortType(qint8 type)
{
    sortType = type;

    for (int i = 0; i < count(); i++) {
        ListItem* item = reinterpret_cast<ListItem*>(this->item(i));
        item->setSortType(sortType);
    }

    sortItems();

    sql::updateSyncStatus("noteSortType", sortType);
}

void NotesWidget::clearSelection()
{
    signalsDisabled = true;
    selectionModel()->clearSelection();
    signalsDisabled = false;
}

void NotesWidget::insertNewNote(QString guid, QString noteBook, QString title)
{
    ListItem* item = new ListItem(this);
    item->setText(title);
    item->setNoteBookGUID(noteBook);
    item->setIcon(QIcon::fromTheme("basket"));
    item->setGUID(guid);

    signalsDisabled = true;
    setCurrentItem(item);
    signalsDisabled = false;
}
