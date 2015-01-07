#include "treeitem.h"

TreeItem::TreeItem(QTreeWidget *view):
    QTreeWidgetItem(view), ratio(0), count(0)
{
  //  setCheckState(0, Qt::Checked);
}

QStringList TreeItem::getGUIDs()
{
    return guid;
}

void TreeItem::appendGUID(QString id)
{
    guid.append(id);
}

QString TreeItem::getParentGuid()
{
    return parentGuid;
}

void TreeItem::setParentGuid(QString id)
{
    parentGuid = id;
}

TreeItem::noteListType TreeItem::getType(){
    return type;
}

void TreeItem::setType(noteListType t)
{
    type = t;
}

void TreeItem::setSortRatio(int value)
{
    ratio = value;
}

int TreeItem::getSortRatio() const
{
    return ratio;
}

bool TreeItem::operator<( const QTreeWidgetItem & other ) const
{
    int column = treeWidget()->sortColumn();

    TreeItem &o = (TreeItem &)other;
    if (getSortRatio() == o.getSortRatio())
        return text( column ).toLower() < other.text( column ).toLower();

    return getSortRatio() < o.getSortRatio();
}

void TreeItem::setName(QString n)
{
    name = n;
}

QString TreeItem::getName()
{
    return name;
}

void TreeItem::setCount(int c) {
    if (c == count)
        return;

    count = c;
    updateText();
}

int TreeItem::getCount() {
    return count;
}

void TreeItem::setText(QString str) {
    if (txt == str)
        return;

    txt = str;
    updateText();
}

QString TreeItem::getText() {
    return txt;
}

void TreeItem::updateText() {
    if (count > 0)
        setText(0, QString("%1 (%2)").arg(txt).arg(count));
    else
        setText(0, txt);
}

TreeItem* TreeItem::parentItem() {
    return reinterpret_cast<TreeItem*>(parent());
}
