#ifndef TREEITEM_H
#define TREEITEM_H

#include <QTreeWidgetItem>
#include <QStringList>



class TreeItem : public QTreeWidgetItem
{
public:
    enum noteListType {
        allNotes = 1,
        noteBook = 2,
        trashBin = 3,
        stack = 4,
        conflict = 5
    };

    TreeItem(QTreeWidget *view = 0);    
    QStringList getGUIDs();
    void appendGUID(QString id);
    QString getParentGuid();
    void setParentGuid(QString id);
    noteListType getType();
    void setType(noteListType t);
    void setSortRatio(int value);
    int getSortRatio() const;
    void setName(QString n);
    QString getName();
    void setCount(int c);
    int getCount();
    void setText(QString str);
    QString getText();
    using QTreeWidgetItem::setText;
    TreeItem *parentItem();

    bool operator<( const QTreeWidgetItem & other ) const;

private:
    QStringList guid;
    noteListType type;
    QString parentGuid;
    QString name;
    int ratio;
    int count;
    QString txt;

    void updateText();
};

#endif // TREEITEM_H
