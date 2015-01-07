#ifndef PDFCACHE_H
#define PDFCACHE_H

#include <QObject>
#include <QHash>

#if QT_VERSION >= 0x050000
#include <poppler-qt5.h>
#else
#include <poppler-qt4.h>
#endif

class pdfCache : public QObject
{
    Q_OBJECT
public:
    pdfCache(QObject *parent = 0);
    void load(QString hash);
    Poppler::Document *getDocument(QString hash);

signals:

public slots:

private:
    QHash<QString, Poppler::Document *> pdfList;
};

#endif // PDFCACHE_H
