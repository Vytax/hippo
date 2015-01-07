#ifndef NOTESWIDGET_H
#define NOTESWIDGET_H

#include "listitem.h"
#include "treeitem.h"

#include <QListWidget>
#include <QDrag>
#include <QDragEnterEvent>

class NotesWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit NotesWidget(QWidget *parent = 0);

    void startDrag( Qt::DropActions supportedActions );
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);

    ListItem *getNoteWithGuid(QString guid);
    void switchNotebook(TreeItem::noteListType type, QStringList id, QString currentNote);
    void switchTag(QStringList id, QString currentNote);
    void clearSelection();
    void insertNewNote(QString guid, QString noteBook, QString title);
    
signals:
    void noteDeleted(QString note);
    void noteRestored(QString note);
    void noteCreated();
    void noteSwitched();
    void reloaded();
    
public slots:

private slots:
    void switchNote();

private:
    void contextMenuEvent ( QContextMenuEvent * event );
    void selectNoteWithGuid(QString guid);
    void setNoteSortType(qint8 type);

    TreeItem::noteListType listType;
    bool signalsDisabled;
    qint8 sortType;
};

#endif // NOTESWIDGET_H
