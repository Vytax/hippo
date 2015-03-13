#ifndef SEARCHQUERYPARSER_H
#define SEARCHQUERYPARSER_H

#include <QObject>
#include <QVariantMap>
#include <QStringList>
#include <QPair>

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
    QPair<QDateTime, QDateTime> parseDate(QString str);
    QStringList getNotesByDate(QPair<QDateTime, QDateTime> interval, QString column);

    QList<QVariantMap> keys;

    QString m_noteindex_query;
    QStringList complexTypes;
    bool any;

};

#endif // SEARCHQUERYPARSER_H
