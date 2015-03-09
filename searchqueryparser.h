#ifndef SEARCHQUERYPARSER_H
#define SEARCHQUERYPARSER_H

#include <QObject>
#include <QVariantMap>

class SearchQueryKey
{
public:
    explicit SearchQueryKey(QString key);
};

class SearchQueryKey_Simple: public SearchQueryKey
{
public:
   // void SearchQueryKey_Simple(QString key) { };
};

class SearchQueryParser : public QObject
{
    Q_OBJECT
public:
    SearchQueryParser(QString query, QObject *parent = 0);

    QStringList getSQLQuery();

signals:

public slots:

private:
    QString m_query;

    void parse();
    void parseKey(QString key);
    bool isQueryComplex();
    QStringList createSimpleQuery();
    QStringList createComplexQuery();
    QStringList getAllNotes();
    QStringList getNoteIndexQuery(QString field, QString key);
    QStringList intersectLists(QStringList a, QStringList b);
    QStringList mergeLists(QStringList a, QStringList b);
    QStringList subtractLists(QStringList a, QStringList b);
    QStringList joinLists(QStringList a, QStringList b, bool intersect);

    QList<QVariantMap> keys;

    QString m_noteindex_query;
    bool any;

};

#endif // SEARCHQUERYPARSER_H
