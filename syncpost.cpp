#include "syncpost.h"
#include "note.h"
#include "tag.h"
#include "notebook.h"
#include "resource.h"
#include "edamprotocol.h"

#include <QEventLoop>
#include <QDebug>
#include <QSqlQuery>

SyncPost::SyncPost(QObject *parent) :
    QObject(parent)
{
    canceled = false;
}

SyncPost::t_updates SyncPost::readUpdates() {
    t_updates result;

    QSqlQuery query;

    query.exec("SELECT notes.guid FROM notes LEFT JOIN notesContent ON notesContent.hash = notes.contentHash WHERE notesContent.hash IS NULL OR notesContent.hash = ''");
    while (query.next()) {
        t_update update;
        update["guid"] = query.value(0).toString();
        update["type"] = "notesContent";

        result.enqueue(update);
    }

    query.clear();
    query.exec("SELECT resources.guid FROM resources LEFT JOIN resourcesData ON resourcesData.hash = resources.bodyHash WHERE resourcesData.data IS NULL OR resourcesData.data = ''");
    while (query.next()) {
        t_update update;
        update["guid"] = query.value(0).toString();
        update["type"] = "resourceData";

        result.enqueue(update);
    }

    query.clear();
    query.exec("SELECT guid FROM notebookUpdates GROUP BY guid");
    while (query.next()) {
        t_update update;
        update["guid"] = query.value(0).toString();
        update["type"] = "notebook";

        result.enqueue(update);
    }

    query.clear();
    query.exec("SELECT guid FROM tagsUpdates GROUP BY guid");
    while (query.next()) {
        t_update update;
        update["guid"] = query.value(0).toString();
        update["type"] = "tag";

        result.enqueue(update);
    }

    query.clear();
    query.exec("SELECT guid FROM noteUpdates GROUP BY guid");
    while (query.next()) {
        t_update update;
        update["guid"] = query.value(0).toString();
        update["type"] = "note";

        result.enqueue(update);
    }

    return result;
}

void SyncPost::getNoteContent(QString guid) {
    Note *n = Note::fromGUID(guid);

    if (n == NULL)
        return;

    if (n->hasData())
        return;

    n->fetchContent();
    delete n;
}

void SyncPost::getResourceData(QString guid) {
    Resource *r = Resource::fromGUID(guid);
    if (r == NULL)
        return;

    if (r->hasData())
        return;

    r->getContent();
    delete r;
}

void SyncPost::updateNote(QString guid) {
    Note *n = new Note();
    if (n->loadFromSQL(guid)) {
        connect(n, SIGNAL(noteGuidChanged(QString,QString)), this, SIGNAL(noteGuidChanged(QString,QString)));
        n->sync();
    }

    delete n;
}

void SyncPost::updateTag(QString guid) {
    Tag *t = new Tag();
    if (t->loadFromSQL(guid))
        t->sync();
    delete t;
}

void SyncPost::updateNoteBook(QString guid) {
    NoteBook *nb = new NoteBook();

    if (nb->loadFromSQL(guid))
        nb->sync();

    delete nb;
}

void SyncPost::sync() {
    qDebug() << "SyncPost::sync()";

    t_updates updates = readUpdates();

    int unsyncNotes = updates.size();

    if (unsyncNotes == 0)
        return;

    int progress = 0;

    while (!updates.isEmpty()) {
        t_update update = updates.dequeue();

        QString type = update["type"];

        if (type == "note")
            updateNote(update["guid"]);
        else if (type == "tag")
            updateTag(update["guid"]);
        else if (type == "notebook")
            updateNoteBook(update["guid"]);
        else if (type == "resourceData")
            getResourceData(update["guid"]);
        else if (type == "notesContent")
            getNoteContent(update["guid"]);

        int p = (int)(((double)(++progress)/unsyncNotes)*1000);
        EdamProtocol::GetInstance()->getSyncEngine()->updateProgress(p, 2);
    }
}

void SyncPost::cancelSync()
{
    canceled = true;
}
