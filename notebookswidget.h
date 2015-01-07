#ifndef NOTEBOOKSWIDGET_H
#define NOTEBOOKSWIDGET_H

#include <QTreeWidget>
#include <QDropEvent>

#include "treeitem.h"

class NotebooksWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit NotebooksWidget(QWidget *parent = 0);

    void dropEvent(QDropEvent * event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    Qt::DropActions supportedDropActions () const;
    void selectNotebookWithName(QString name);

    TreeItem *defaultNotebook();
    void contextMenuEvent (QContextMenuEvent * event);
    void updateCounts();
    
signals:
    void noteMoved(QString note, QString notebook);
    void noteDeleted(QString note);
    void noteRestored(QString note, QString notebook);
    
public slots:
    void reload();

private:
    TreeItem *defaultNotebookItem;

    int notesCountInNoteBook(TreeItem::noteListType type, QStringList id = QStringList());
    void renameNotebook(TreeItem *item);
    QStringList notebooksNames();
    void renameStack(QString oldName, QString newName);
    void newStack(TreeItem *item);
    QStringList stacksList();
    void addToStack(TreeItem *item, QString stack);
    void newNotebook();
    int conflictsCount();
};

#endif // NOTEBOOKSWIDGET_H
