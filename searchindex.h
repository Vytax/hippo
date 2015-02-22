#ifndef SEARCHINDEX_H
#define SEARCHINDEX_H

#include <QObject>
#include <QStringList>

class SearchIndex : public QObject
{
    Q_OBJECT
public:
    explicit SearchIndex(QObject *parent = 0);

    QStringList search(QString query);

signals:

public slots:
    void buildSearchIndex();

private:
    QStringList getNotesList();

    void writeNoteIndex(QString guid, QString title, QString content);
    void clearIndex();

};

#endif // SEARCHINDEX_H
