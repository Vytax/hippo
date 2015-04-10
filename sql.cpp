#include "sql.h"
#include "Logger.h"

#include <QStringList>
#include <QVariant>
#include <QUuid>
#include <QDir>
#include <QSqlError>

#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

QString newGUID(QString table, QString column) {

    if (table.isEmpty() || column.isEmpty())
        return QUuid::createUuid().toString().mid(1,36);

    for (QString uuid = QUuid::createUuid().toString().mid(1,36);; uuid = QUuid::createUuid().toString().mid(1,36)) {        
        QSqlQuery query;
        query.prepare(QString("SELECT COUNT(*) FROM %1 WHERE %2=:key").arg(table).arg(column));
        query.bindValue(":key", uuid);
        query.exec();

        if (!query.next())
            return "";

        if (query.value(0).toInt() == 0)
            return uuid;
    }
    return "";
}

QString AddSQLArray(int count)
{
    if (count <= 0)
        return QString();

    QString result = ":value0";
    for (int i=1; i<count; i++)
        result+= QString(", :value%1").arg(i);

    return result;
}

void BindSQLArray(QSqlQuery &query, QStringList values)
{
    for (int i=0; i<values.count(); i++)
        query.bindValue(QString(":value%1").arg(i), values.at(i));
}

sql::sql(QObject *parent) :
    QObject(parent)
{
#if QT_VERSION >= 0x050000
    QString dataDir = QStandardPaths::standardLocations(QStandardPaths::DataLocation).at(0);
#else
    QString dataDir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif

    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    db = QSqlDatabase::addDatabase("QSQLITE");
    if (!db.isValid()) {
        LOG_ERROR("DB driver load failed.");
        return;
    }

    QString dbFile = dataDir + QDir::separator() + "db.sql";
    db.setDatabaseName(dbFile);
    if (!db.open())
        LOG_ERROR("DB load failed.");

    LOG_INFO("Opened database: " + dbFile);
}

sql::~sql()
{
    db.close();
}

bool sql::test() {
    QSqlQuery query;
    return query.exec("SELECT * FROM syncStatus");
}

void sql::dropTables()
{
    db.exec("DROP TABLE notes");
    db.exec("DROP TABLE notesTags");
    db.exec("DROP TABLE notebooks");
    db.exec("DROP TABLE tags");
    db.exec("DROP TABLE resources");
    db.exec("DROP TABLE noteAttributes");
    db.exec("DROP TABLE noteIndex");
    db.exec("DROP TABLE noteIndexGUIDs");
    checkTables();
}

void sql::updateSyncStatus(QString key, QVariant value)
{
    QSqlQuery query;
    query.prepare("REPLACE INTO syncStatus (option, value) VALUES (:key, :value)");
    query.bindValue(":key", key);
    query.bindValue(":value", value);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());

}

QVariant sql::readSyncStatus(QString key, QVariant defaultValue)
{
    QSqlQuery query;
    query.prepare("SELECT value FROM syncStatus WHERE option=:key");
    query.bindValue(":key", key);
    if (!query.exec())
        LOG_ERROR("SQL: " + query.lastError().text());

    if (query.next())
        return query.value(0);

    return defaultValue;
}

void sql::checkTables()
{
    migrate();

    QSqlQuery query;

    QString notesTable("CREATE TABLE notes ( ");
    notesTable +="guid VARCHAR(36) NOT NULL PRIMARY KEY, ";
    notesTable +="title TEXT, ";
    notesTable +="contentHash VARCHAR(16), ";
    notesTable +="created UNSIGNED BIG INT DEFAULT 0, ";
    notesTable +="updated UNSIGNED BIG INT DEFAULT 0, ";
    notesTable +="deleted UNSIGNED BIG INT DEFAULT 0, ";
    notesTable +="active BOOLEAN DEFAULT true, ";
    notesTable +="updateSequenceNum INTEGER DEFAULT 0, ";
    notesTable +="notebookGuid VARCHAR(36) NOT NULL";
    notesTable +=")";
    if (!db.tables().contains("notes"))
        if (!query.exec(notesTable))
            LOG_ERROR("SQL: " + query.lastError().text());

    QString conflictingNotes("CREATE TABLE conflictingNotes ( ");
    conflictingNotes +="guid VARCHAR(36) NOT NULL PRIMARY KEY, ";
    conflictingNotes +="contentHash VARCHAR(16), ";
    conflictingNotes +="updated UNSIGNED BIG INT DEFAULT 0 ";
    conflictingNotes +=")";
    if (!db.tables().contains("conflictingNotes"))
        if (!query.exec(conflictingNotes))
            LOG_ERROR("SQL: " + query.lastError().text());

    QString notesContent("CREATE TABLE notesContent ( ");
    notesContent +="hash VARCHAR(32) NOT NULL PRIMARY KEY,";
    notesContent +="content TEXT, ";
    notesContent +="length INTEGER DEFAULT 0";
    notesContent +=")";
    if (!db.tables().contains("notesContent"))
        if (!query.exec(notesContent))
                LOG_ERROR("SQL: " + query.lastError().text());

    QString notesTagsTable("CREATE TABLE notesTags ( ");
    notesTagsTable +="id INTEGER PRIMARY KEY AUTOINCREMENT, ";
    notesTagsTable +="noteGuid VARCHAR(36) NOT NULL, ";
    notesTagsTable +="guid VARCHAR(36) NOT NULL,";
    notesTagsTable +="UNIQUE(noteGuid, guid) ON CONFLICT REPLACE )";
    if (!db.tables().contains("notesTags"))
        if (!query.exec(notesTagsTable))
            LOG_ERROR("SQL: " + query.lastError().text());

    QString notebooksTable("CREATE TABLE notebooks ( ");
    notebooksTable +="guid VARCHAR(36) NOT NULL PRIMARY KEY, ";
    notebooksTable +="name TEXT, ";
    notebooksTable +="updateSequenceNum INTEGER DEFAULT -1, ";
    notebooksTable +="defaultNotebook BOOLEAN DEFAULT FALSE, ";
    notebooksTable +="serviceCreated UNSIGNED BIG INT DEFAULT 0, ";
    notebooksTable +="serviceUpdated UNSIGNED BIG INT DEFAULT 0, ";
    notebooksTable +="stack TEXT )";
    if (!db.tables().contains("notebooks"))
        if (!query.exec(notebooksTable))
            LOG_ERROR("SQL: " + query.lastError().text());

    QString tagsTable("CREATE TABLE tags ( ");
    tagsTable +="guid VARCHAR(36) NOT NULL PRIMARY KEY, ";
    tagsTable +="name TEXT, ";
    tagsTable +="parentGuid VARCHAR(36), ";
    tagsTable +="updateSequenceNum INTEGER DEFAULT -1 )";
    if (!db.tables().contains("tags"))
        if (!query.exec(tagsTable))
            LOG_ERROR("SQL: " + query.lastError().text());

    QString resourcesTable("CREATE TABLE resources ( ");
    resourcesTable +="guid VARCHAR(36) NOT NULL PRIMARY KEY, ";
    resourcesTable +="noteGuid VARCHAR(36), ";
    resourcesTable +="bodyHash VARCHAR(32), ";
    resourcesTable +="width INTEGER DEFAULT 0, ";
    resourcesTable +="height INTEGER DEFAULT 0, ";
    resourcesTable +="new BOOLEAN DEFAULT FALSE, ";
    resourcesTable +="updateSequenceNum INTEGER DEFAULT -1, ";
    resourcesTable +="size INTEGER DEFAULT 0, ";
    resourcesTable +="mime VARCHAR(255), ";
    resourcesTable +="fileName TEXT, ";
    resourcesTable +="sourceURL TEXT, ";
    resourcesTable +="attachment BOOLEAN DEFAULT FALSE, ";
    resourcesTable +="UNIQUE(noteGuid, bodyHash) ON CONFLICT REPLACE )";
    if (!db.tables().contains("resources"))
        if (!query.exec(resourcesTable))
            LOG_ERROR("SQL: " + query.lastError().text());

    QString dataTable("CREATE TABLE resourcesData ( ");
    dataTable +="hash VARCHAR(32) NOT NULL PRIMARY KEY, ";
    dataTable +="data BLOB)";
    if (!db.tables().contains("resourcesData"))
        if (!query.exec(dataTable))
            LOG_ERROR("SQL: " + query.lastError().text());

    QString syncStatusTable("CREATE TABLE syncStatus ( ");
    syncStatusTable +="option TEXT NOT NULL PRIMARY KEY, ";
    syncStatusTable +="value UNSIGNED BIG INT DEFAULT 0 )";
    if (!db.tables().contains("syncStatus"))
        if (!query.exec(syncStatusTable))
            LOG_ERROR("SQL: " + query.lastError().text());

    QString noteUpdates("CREATE TABLE noteUpdates ( ");
    noteUpdates += "id INTEGER PRIMARY KEY AUTOINCREMENT, ";
    noteUpdates += "guid VARCHAR(36) NOT NULL, ";
    noteUpdates += "field INT DEFAULT 0,";
    noteUpdates += "UNIQUE(guid, field) ON CONFLICT REPLACE )";
    if (!db.tables().contains("noteUpdates")) {
        if (!query.exec(noteUpdates))
            LOG_ERROR("SQL: " + query.lastError().text());
    }

    QString noteAttributes("CREATE TABLE noteAttributes ( ");
    noteAttributes += "id INTEGER PRIMARY KEY AUTOINCREMENT, ";
    noteAttributes += "noteGuid VARCHAR(36) NOT NULL, ";
    noteAttributes += "field TEXT,";
    noteAttributes += "value TEXT,";
    noteAttributes += "UNIQUE(noteGuid, field) ON CONFLICT REPLACE )";
    if (!db.tables().contains("noteAttributes")) {
        if (!query.exec(noteAttributes))
            LOG_ERROR("SQL: " + query.lastError().text());
    }

    QString tagsUpdates("CREATE TABLE tagsUpdates ( ");
    tagsUpdates += "id INTEGER PRIMARY KEY AUTOINCREMENT, ";
    tagsUpdates += "guid VARCHAR(36) NOT NULL, ";
    tagsUpdates += "field INT DEFAULT 0,";
    tagsUpdates += "UNIQUE(guid, field) ON CONFLICT REPLACE )";
    if (!db.tables().contains("tagsUpdates")) {
        if (!query.exec(tagsUpdates))
            LOG_ERROR("SQL: " + query.lastError().text());
    }

    QString notebookUpdates("CREATE TABLE notebookUpdates ( ");
    notebookUpdates += "id INTEGER PRIMARY KEY AUTOINCREMENT, ";
    notebookUpdates += "guid VARCHAR(36) NOT NULL, ";
    notebookUpdates += "field INT DEFAULT 0,";
    notebookUpdates += "UNIQUE(guid, field) ON CONFLICT REPLACE )";
    if (!db.tables().contains("notebookUpdates")) {
        if (!query.exec(notebookUpdates))
            LOG_ERROR("SQL: " + query.lastError().text());
    }

    QString noteIndex("CREATE VIRTUAL TABLE noteIndex USING fts3(title, content, tags, notebook);");
    if (!db.tables().contains("noteIndex")) {
        if (!query.exec(noteIndex))
            LOG_ERROR("SQL: " + query.lastError().text());
    }

    QString noteIndexGUIDs("CREATE TABLE noteIndexGUIDs (docid INTEGER PRIMARY KEY, guid VARCHAR(36) NOT NULL);");
    if (!db.tables().contains("noteIndexGUIDs")) {
        if (!query.exec(noteIndexGUIDs))
            LOG_ERROR("SQL: " + query.lastError().text());
    }

}

void sql::migrate() {
    QSqlQuery query;
    query.exec("PRAGMA table_info(noteIndex);");

    int colCount = 0;

    while (query.next())
        colCount++;

    if (colCount < 4) {
        db.exec("DROP TABLE noteIndex");
        db.exec("DROP TABLE noteIndexGUIDs");
    }
}
