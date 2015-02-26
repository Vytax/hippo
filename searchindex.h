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
    void dropIndex();


signals:

public slots:
    void buildSearchIndex();
    void updateNoteIndex(QString guid);
    void dropNoteIndex(QString guid);

private:
    QStringList getUnindexedNotesList();

    void writeNoteIndex(QString guid, QString title, QString content);


};

#endif // SEARCHINDEX_H
