#ifndef CUSTOMWEBVIEW_H
#define CUSTOMWEBVIEW_H

#include <QWebView>
#include <QDragMoveEvent>

class CustomWebView : public QWebView
{
    Q_OBJECT
public:
    explicit CustomWebView(QWidget *parent = 0);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent * event);

signals:
    void tagUpdated(QString guid, bool checked);
    void fileInserted(QString fileName);

public slots:
private:
    QVariant JS(QString command);
    QString getCurrentNoteGuid();
    QStringList fileTypes;

};

#endif // CUSTOMWEBVIEW_H
