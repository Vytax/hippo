#include "notebookswidget.h"
#include "listitem.h"
#include "sql.h"
#include "renamedialog.h"
#include "notebook.h"

#include <QTreeWidgetItemIterator>
#include <QSqlQuery>
#include <QMenu>
#include <QMimeData>

#include <QDebug>

NotebooksWidget::NotebooksWidget(QWidget *parent) :
    QTreeWidget(parent)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    defaultNotebookItem = NULL;
}

void NotebooksWidget::dropEvent(QDropEvent * event)
{
    if (event->mimeData()->hasFormat("qevernote/note")) {

        QString data = QString::fromLatin1(event->mimeData()->data("qevernote/note"));

        QStringList parts = data.split('@');
        QString noteGuid = parts[0];
        bool active = (parts[2] == "active");

        TreeItem* item = reinterpret_cast<TreeItem*>(itemAt(event->pos()));

        if (item != NULL) {
            if (item->getType() == TreeItem::trashBin) {
                if (!active) {
                    event->ignore();
                    return;
                }
                emit noteDeleted(noteGuid);
            }
            else {
                if (active)
                    emit noteMoved(noteGuid, item->getGUIDs().first());
                else
                    emit noteRestored(noteGuid, item->getGUIDs().first());
            }
        }
    }

    event->ignore();
}

void NotebooksWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("qevernote/note"))
        event->accept();
    else
        event->ignore();
}

void NotebooksWidget::dragMoveEvent(QDragMoveEvent *event)
{
    TreeItem* item = reinterpret_cast<TreeItem*>(itemAt(event->pos()));

    if (!event->mimeData()->hasFormat("qevernote/note")) {
        event->ignore();
        return;
    }

    if (item == NULL) {
        event->ignore();
        return;
    }

    item->setExpanded(true);

    if ((item->getType() == TreeItem::allNotes) || (item->getType() == TreeItem::stack)) {
        event->ignore();
        return;
    }

    QString data = QString::fromLatin1(event->mimeData()->data("qevernote/note"));

    QStringList parts = data.split('@');
    QString oldNoteBook = parts[1];
    bool active = (parts[2] == "active");

    if ((item->getType() == TreeItem::trashBin) && !active) {
        event->ignore();
        return;
    }

    if (!item->getGUIDs().isEmpty() && (oldNoteBook == item->getGUIDs().first()) && active) {
        event->ignore();
        return;
    }

    event->setDropAction(Qt::MoveAction);
    event->accept();
}

Qt::DropActions NotebooksWidget::supportedDropActions () const
{
    return Qt::MoveAction;
}

void NotebooksWidget::selectNotebookWithName(QString name)
{
    QTreeWidgetItemIterator it(this);
    while (*it) {
        TreeItem* item = reinterpret_cast<TreeItem*>(*it);
        if (item->getName() == name) {
            setCurrentItem(item);
            return;
        }
        ++it;
    }
}

void NotebooksWidget::reload()
{
    qDebug() << "reloadNotebooks()";

    clear();
    defaultNotebookItem = NULL;

    QHash<QString, TreeItem*> stacks;
    QHash<QString, TreeItem*> items;

    TreeItem* allnotes = new TreeItem(this);
    allnotes->setText("All Notes");
    allnotes->setCount(notesCountInNoteBook(TreeItem::allNotes));
    allnotes->setIcon(0, QIcon::fromTheme("folder-orange"));
    allnotes->setType(TreeItem::allNotes);
    allnotes->setName("allnotes");
    allnotes->setSortRatio(-100);
    items[allnotes->getName()] = allnotes;

    QSqlQuery result;

    result.exec("SELECT name, guid, stack, defaultNotebook FROM notebooks");

    while (result.next()) {
        TreeItem* item = new TreeItem();
        item->setType(TreeItem::noteBook);
        QString id = result.value(1).toString();
        bool defaultNotebook = result.value(3).toBool();
        item->setText(result.value(0).toString());
        item->setCount(notesCountInNoteBook(TreeItem::noteBook, QStringList(id)));
        item->appendGUID(id);
        item->setType(TreeItem::noteBook);
        item->setName(QString("notebook@%1").arg(id));
        items[item->getName()] = item;

        if (defaultNotebook) {
            item->setIcon(0, QIcon::fromTheme("folder-favorites"));
            defaultNotebookItem = item;
        }
        else
            item->setIcon(0, QIcon::fromTheme("folder"));

        QString st = result.value(2).toString();
        if (st.isEmpty())
            addTopLevelItem(item);
        else {
            if (!stacks.contains(st)){
                TreeItem* sitem = new TreeItem(this);
                sitem->setText(st);
                sitem->setIcon(0, QIcon::fromTheme("folder-green"));
                sitem->setType(TreeItem::stack);
                sitem->setName(QString("stack@%1").arg(QString(st.toLatin1().toBase64())));
                stacks[st] = sitem;
                items[sitem->getName()] = sitem;
            }
            stacks[st]->addChild(item);
            stacks[st]->appendGUID(id);
        }
    }

    int conflictsC = conflictsCount();
    if (conflictsC > 0) {
        TreeItem* conflict = new TreeItem(this);
        conflict->setText(QString("Conflicting Changes (%1)").arg(conflictsC));
        conflict->setIcon(0, QIcon::fromTheme("folder-important"));
        conflict->setType(TreeItem::conflict);
        conflict->setSortRatio(99);
    }

    TreeItem* item = new TreeItem(this);
    item->setText("Trash");
    item->setCount(notesCountInNoteBook(TreeItem::trashBin));
    item->setIcon(0, QIcon::fromTheme("user-trash"));
    item->setType(TreeItem::trashBin);
    item->setSortRatio(100);
    item->setName("trash");

    expandAll();

    setCurrentItem(allnotes);
}

int NotebooksWidget::notesCountInNoteBook(TreeItem::noteListType type, QStringList id)
{
    QSqlQuery count;
    if (type == TreeItem::allNotes){
        count.prepare("SELECT COUNT(*) FROM notes WHERE active=:active");
        count.bindValue(":active", true);
    } else if (type == TreeItem::noteBook) {
        count.prepare(QString("SELECT COUNT(*) FROM notes WHERE notebookGuid IN (%1) AND active=:active").arg(AddSQLArray(id.count())));
        BindSQLArray(count, id);
        count.bindValue(":active", true);
    } else if (type == TreeItem::trashBin) {
        count.prepare("SELECT COUNT(*) FROM notes WHERE active=:active");
        count.bindValue(":active", false);
    } else
        return 0;

    count.exec();

    if (count.next())
        return count.value(0).toInt();

    return 0;
}

void NotebooksWidget::updateCounts() {
    QTreeWidgetItemIterator it(this);
    while (*it) {
        TreeItem* item = reinterpret_cast<TreeItem*>(*it);
        item->setCount(notesCountInNoteBook(item->getType(), item->getGUIDs()));
        ++it;
    }
}

TreeItem* NotebooksWidget::defaultNotebook()
{
    return defaultNotebookItem;
}

void NotebooksWidget::contextMenuEvent (QContextMenuEvent * event)
{
    qDebug() << "contextMenuEvent";
    TreeItem* item = reinterpret_cast<TreeItem*>(this->itemAt(event->pos()));

    QMenu menu(this);

    if (item != NULL) {
        TreeItem::noteListType type = item->getType();
        if (type == TreeItem::allNotes) {
            QAction *newNotebook = new QAction(QIcon::fromTheme("document-new"), "New Notebook...", this);
            newNotebook->setObjectName("newNotebook");
            menu.addAction(newNotebook);
        }
        if ((type == TreeItem::noteBook) || (type == TreeItem::stack)){
            QAction *rename = new QAction(QIcon::fromTheme("edit-rename"), "Rename...", this);
            rename->setObjectName("rename");
            menu.addAction(rename);
        }
        if (type == TreeItem::stack) {
            QAction *deleteStack = new QAction(QIcon::fromTheme("edit-delete"), "Delete Stack", this);
            deleteStack->setObjectName("deleteStack");
            menu.addAction(deleteStack);
        }
        if (type == TreeItem::noteBook) {
            QMenu *stMenu = menu.addMenu("Add To Stack");

            QStringList stacks = stacksList();
            QString stack;

            foreach (stack, stacks) {
                QAction *s = new QAction(stack, this);
                s->setObjectName("stack");
                s->setData(stack);
                stMenu->addAction(s);

                TreeItem *parent = item->parentItem();
                if (parent != NULL) {
                    s->setDisabled(parent->getText() == stack);
                }
            }

            if (!stMenu->isEmpty())
                stMenu->addSeparator();

            QAction *newStack = new QAction("New Stack...", this);
            newStack->setObjectName("newStack");
            stMenu->addAction(newStack);
        }
    } else {
        QAction *newNotebook = new QAction(QIcon::fromTheme("document-new"), "New Notebook...", this);
        newNotebook->setObjectName("newNotebook");
        menu.addAction(newNotebook);
    }

    if (menu.isEmpty()) {
        event->ignore();
        return;
    }

    QAction *ret = menu.exec(event->globalPos());

    if (ret == NULL) {
        event->ignore();
        return;
    }

    if (ret->objectName() == "rename")
        renameNotebook(item);
    else if (ret->objectName() == "deleteStack") {
        renameStack(item->getText(), "");
        reload();
    } else if (ret->objectName() == "newStack")
        newStack(item);
    else if (ret->objectName() == "stack")
        addToStack(item, ret->data().toString());
    else if (ret->objectName() == "newNotebook")
        newNotebook();
}

void NotebooksWidget::renameNotebook(TreeItem *item) {
    if (item == NULL)
        return;

    QString guid = item->getGUIDs().first();
    if (guid.isEmpty())
        return;

    TreeItem::noteListType type = item->getType();
    if ((type != TreeItem::noteBook) && (type != TreeItem::stack))
        return;

    RenameDialog dialog(this);

    if (type == TreeItem::noteBook) {
        dialog.setWindowTitle("Rename Notebook");
        dialog.setLabel("Notebook name");

        QStringList nNames = notebooksNames();
        nNames.removeAll(item->getText());
        dialog.setBlocked(nNames, "A Notebook with that name already exists!");
    } else if (type == TreeItem::stack) {
        dialog.setWindowTitle("Rename Stack");
        dialog.setLabel("Stack name");
    }
    dialog.setText(item->getText());

    if (dialog.exec() != QDialog::Accepted)
        return;

    QString newName = dialog.getText();
    QString oldName = item->getText();

    if (newName == oldName)
        return;

    item->setText(newName);

    if (type == TreeItem::noteBook) {
        NoteBook *nb = new NoteBook();
        if (nb->loadFromSQL(guid)) {
            NoteBook::NoteBookUpdates updates;
            updates[NoteBook::T_NAME] = newName;
            updates[NoteBook::T_UPDATED] = QDateTime::currentDateTime();
            nb->update(updates);
        }
        delete nb;
    } else if (type == TreeItem::stack) {
        renameStack(oldName, newName);
    }
}

QStringList NotebooksWidget::notebooksNames() {
    QStringList result;

    QSqlQuery query;
    query.exec("SELECT name FROM notebooks GROUP BY name");

    while (query.next())
        result.append(query.value(0).toString());

    return result;
}

void NotebooksWidget::renameStack(QString oldName, QString newName) {

    QSqlQuery query;
    query.prepare("SELECT guid FROM notebooks WHERE stack=:stack COLLATE NOCASE");
    query.bindValue(":stack", oldName);
    query.exec();

    QStringList notebooks;
    while (query.next()) {
        notebooks.append(query.value(0).toString());
    }

    qDebug() << notebooks;

    query.clear();
    query.prepare("UPDATE notebooks SET stack=:newstack WHERE stack=:stack COLLATE NOCASE");
    query.bindValue(":stack", oldName);
    query.bindValue(":newstac", newName);
    query.exec();

    QString guid;
    foreach (guid, notebooks) {
        NoteBook *nb = new NoteBook();
        if (nb->loadFromSQL(guid)) {
            NoteBook::NoteBookUpdates updates;
            updates[NoteBook::T_STACK] = newName;
            updates[NoteBook::T_UPDATED] = QDateTime::currentDateTime();
            nb->update(updates);
        }
        delete nb;
    }
}

void NotebooksWidget::newStack(TreeItem *item) {
    RenameDialog dialog(this);
    dialog.setWindowTitle("Create a New Stack");
    dialog.setLabel("Stack name");

    if (dialog.exec() != QDialog::Accepted)
        return;

    addToStack(item, dialog.getText());
}

void NotebooksWidget::addToStack(TreeItem *item, QString stack) {
    if (item == NULL)
        return;

    QString guid = item->getGUIDs().first();
    if (guid.isEmpty())
        return;

    QString name = item->getName();

    NoteBook *nb = new NoteBook();
    if (nb->loadFromSQL(guid)) {
        NoteBook::NoteBookUpdates updates;
        updates[NoteBook::T_STACK] = stack;
        updates[NoteBook::T_UPDATED] = QDateTime::currentDateTime();
        nb->update(updates);
        reload();
        selectNotebookWithName(name);
    }
    delete nb;
}

QStringList NotebooksWidget::stacksList() {
    QStringList result;

    QSqlQuery query;
    query.exec("SELECT stack FROM notebooks GROUP BY stack");

    while (query.next()) {
        QString item = query.value(0).toString();
        if (!item.isEmpty())
            result.append(item);
    }

    return result;
}

void NotebooksWidget::newNotebook() {
    RenameDialog dialog(this);
    dialog.setWindowTitle("Create a New Notebook");
    dialog.setLabel("Notebook name");
    dialog.setBlocked(notebooksNames(), "A Notebook with that name already exists!");

    if (dialog.exec() != QDialog::Accepted)
        return;

    NoteBook *nb = new NoteBook();
    QString guid = nb->createNewNotebook(dialog.getText());
    delete nb;

    reload();
    selectNotebookWithName(QString("notebook@%1").arg(guid));
}

int NotebooksWidget::conflictsCount() {
    QSqlQuery query;
    query.exec("SELECT COUNT(*) FROM conflictingNotes");

    if (!query.next())
        return 0;

    return query.value(0).toInt();
}
