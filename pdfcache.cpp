#include "pdfcache.h"
#include "resource.h"

#include <QVariant>

#include <QDebug>

pdfCache::pdfCache(QObject *parent) :
    QObject(parent)
{
}

void pdfCache::load(QString hash) {
    Resource *res = Resource::fromHash(hash);
    if (res == NULL)
        return;

    QString mime = res->mimeType();
    QByteArray data = res->getData();
    delete res;

    qDebug() << "PDF" << hash;

    if (mime != "application/pdf")
        return;

    Poppler::Document *doc = Poppler::Document::loadFromData(data);
    doc->setRenderHint(Poppler::Document::Antialiasing, true);
    doc->setRenderHint(Poppler::Document::TextHinting, true);
    doc->setRenderHint(Poppler::Document::TextAntialiasing, true);
    pdfList[hash] = doc;
}

Poppler::Document *pdfCache::getDocument(QString hash){
    if (!pdfList.contains(hash))
        load(hash);

    if (!pdfList.contains(hash))
        return NULL;

    return pdfList[hash];
}
