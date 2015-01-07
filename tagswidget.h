#ifndef TAGSWIDGET_H
#define TAGSWIDGET_H


#include "treeitem.h"
#include <QTreeWidget>
#include <QContextMenuEvent>

class TagsWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit TagsWidget(QWidget *parent = 0);
    void selectTagWithGUID(QString name);
    Qt::DropActions supportedDropActions () const;
    void dragMoveEvent(QDragMoveEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent * event);
    void startDrag( Qt::DropActions supportedActions );

    void updateCounts();
    QStringList currentGuids();

public slots:
    void reload();

signals:
    void tagAdded(QString noteGuid, QString tagGuid);
    void tagsUpdated();

private:
    bool signalsDisabled;
    Qt::ItemFlags itemsFlag;

    int notesCountWithTag(QStringList id);
    void contextMenuEvent ( QContextMenuEvent * event );
    void renameTag(TreeItem *item);
    void removeAll(TreeItem *item);
    
};

#endif // TAGSWIDGET_H
