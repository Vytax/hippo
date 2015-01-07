#ifndef PDFREPLY_H
#define PDFREPLY_H

#include "pdfcache.h"

#include <QNetworkReply>
#include <QFutureWatcher>

class PdfReply : public QNetworkReply
{
    Q_OBJECT
public:
    PdfReply(QObject* parent, const QNetworkRequest& request, pdfCache* pdfdoc);
    ~PdfReply();

    void abort();
    qint64 bytesAvailable() const;
    bool isSequential() const;

protected:
    qint64 readData(char *data, qint64 maxSize);

private:
    QString hash;
    int pageNum;
    QByteArray m_buffer;
    qint64 offset;
    QFutureWatcher<QByteArray> *watcher;

    pdfCache* pdf;

    static QByteArray load(int page, Poppler::Document *doc);

private slots:
     void loadDone();

};

#endif // PDFREPLY_H
