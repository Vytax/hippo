#include "searchqueryparser.h"
#include "Logger.h"

#include <QStringList>
#include <QRegExp>
#include <QSqlQuery>
#include <QSqlError>

SearchQueryParser::SearchQueryParser(QString query, QObject *parent) :
    QObject(parent)
{
    m_noteindex_query = QString("SELECT noteIndexGUIDs.guid FROM noteIndex LEFT JOIN noteIndexGUIDs ON noteIndexGUIDs.docid = noteIndex.docid WHERE ");

    m_query = query;

    LOG_DEBUG(m_query);

    parse();
}

void SearchQueryParser::parse() {

    int keyStart = 0;
    int keyLength = 0;
    bool quoted = false;
    bool quoteEnd = false;
    any = false;

    for (int i = 0; i< m_query.length(); i++) {
        if ((m_query.at(i).isSpace() && !quoted) || quoteEnd) {
            if (keyLength > 0) {
                QString key = m_query.mid(keyStart, keyLength);
                parseKey(key);
            }
            keyStart = i + 1;
            keyLength = 0;
            quoteEnd = false;
        } else {
            if (m_query.at(i) == '"') {
                quoteEnd = quoted;
                quoted = !quoted;
            }
            keyLength++;
        }
    }

    if (quoted)
        LOG_WARNING("Malformed search query.");

    if (keyStart < m_query.length()) {
        QString key = m_query.mid(keyStart);
        parseKey(key);
    }
}

void SearchQueryParser::parseKey(QString key) {
    key = key.trimmed();

    if (key.isEmpty())
        return;

    bool keyNOT = false;
    QString keyType;
    bool keyQuoted = false;

    if (key.at(0) == '-') {
        keyNOT = true;
        key.remove(0, 1);
    }

    int colon = key.indexOf(':');
    int firstQuote = key.indexOf('"');
    if ((colon > 0) && ((colon < firstQuote) || (firstQuote < 0))) {
        keyType = key.left(colon);
        key.remove(0, colon + 1);
        keyType = keyType.trimmed().toLower();
    }

    if (!key.isEmpty()) {
        if ((key.at(0) == '"') && (key.at(key.size() - 1) == '"')) {
            keyQuoted = true;
        } else if (firstQuote >= 0) {
            key.replace("\"","\\\"");
        }
    }

    if (keyType == "any")
        any = true;

    QVariantMap result;
    result["keyNOT"] = keyNOT;
    result["keyType"] = keyType;
    result["key"] = key;
    result["keyQuoted"] = keyQuoted;

    keys.append(result);
    LOG_DEBUG("keyNOT: " + QVariant(keyNOT).toString() + " type: " + keyType + " key: " + key);
}

QStringList SearchQueryParser::getSQLQuery() {

    LOG_DEBUG("isQueryComplex() " + QVariant(isQueryComplex()).toString());

    if (!isQueryComplex())
        return createSimpleQuery();
    else
        return createComplexQuery();

}

bool SearchQueryParser::isQueryComplex() {

    if (any)
        return true;

    QVariantMap key;

    bool hasKeyNot = true;

    foreach (key, keys) {
        if (key["key"] == "*")
            return true;

        if (key["keyNOT"].toBool() && !key["keyType"].toString().isEmpty())
            return true;

        if (key["keyQuoted"].toBool() && !key["keyType"].toString().isEmpty())
            return true;

        hasKeyNot = hasKeyNot && key["keyNOT"].toBool();
    }

    return hasKeyNot;
}

QStringList SearchQueryParser::createSimpleQuery() {
    QVariantMap key;
    QStringList simpleQuery;
    QStringList guids;

    foreach (key, keys) {
        if (key.isEmpty())
            continue;

        QString k = key["key"].toString();

        if (key["keyType"].toString() == "intitle")
            k = "title:" + k;

        if (key["keyNOT"].toBool())
            k = "-" + k;

        simpleQuery.append(k);
    }

    if (simpleQuery.isEmpty())
        return guids;

    QSqlQuery sql;

    sql.prepare(m_noteindex_query + "noteIndex MATCH :query");
    sql.bindValue(":query", simpleQuery.join(" "));
    if (!sql.exec())
        LOG_ERROR("SQL: " + sql.lastError().text());

    while (sql.next())
        guids.append(sql.value(0).toString());

    return guids;
}

QStringList SearchQueryParser::createComplexQuery() {
    QStringList guids;
    QStringList notGuids;

    bool intersect;
    bool notOnly = true;

    for (int i=0; i < keys.size(); i++) {

        intersect = (i != 0) && !any;
        QVariantMap k = keys.at(i);
        notOnly = notOnly && k["keyNOT"].toBool();

        QString key = k["key"].toString();
        bool keyNot = k["keyNOT"].toBool();
        QString type = k["keyType"].toString();

        if (key.isEmpty())
            continue;

        if (key == "*") {
            guids = joinLists(guids, getAllNotes(), intersect);
            continue;
        }

        if (type == "intitle") {
            if (keyNot)
                notGuids = mergeLists(notGuids, getNoteIndexQuery("title", key));
            else
                guids = joinLists(guids, getNoteIndexQuery("title", key), intersect);
        } else {
            if (keyNot)
                notGuids = mergeLists(notGuids, getNoteIndexQuery("noteIndex", key));
            else
                guids = joinLists(guids, getNoteIndexQuery("noteIndex", key), intersect);
        }

    }

    if (notOnly)
        guids = getAllNotes();

    if (!notGuids.isEmpty()) {
        guids = subtractLists(guids, notGuids);
    }

    return guids;
}

QStringList SearchQueryParser::getAllNotes() {
    QStringList guids;

    QSqlQuery sql;
    if (!sql.exec("SELECT guid FROM notes WHERE active='true';"))
        LOG_ERROR("SQL: " + sql.lastError().text());

    while (sql.next())
        guids.append(sql.value(0).toString());

    return guids;
}

QStringList SearchQueryParser::intersectLists(QStringList a, QStringList b) {
    QStringList result;
    QString str;
    foreach (str, a) {
        if (b.contains(str, Qt::CaseInsensitive))
            result.append(str);
    }
    return result;
}

QStringList SearchQueryParser::mergeLists(QStringList a, QStringList b) {
    QStringList result;
    result.append(a);
    result.append(b);
    result.removeDuplicates();
    return result;
}

QStringList SearchQueryParser::subtractLists(QStringList a, QStringList b) {
    QStringList result = a;
    QString str;
    foreach (str, b) {
        result.removeAll(str);
    }
    return result;
}

QStringList SearchQueryParser::joinLists(QStringList a, QStringList b, bool intersect) {
    if (intersect)
        return intersectLists(a, b);
    else
        return mergeLists(a, b);
}


QStringList SearchQueryParser::getNoteIndexQuery(QString field, QString key) {
    QStringList guids;

    QSqlQuery sql;
    sql.prepare(m_noteindex_query + field + " MATCH :key;");
    sql.bindValue(":key", key);
    if (!sql.exec())
        LOG_ERROR("SQL: " + sql.lastError().text());

    while (sql.next())
        guids.append(sql.value(0).toString());

    return guids;
}
