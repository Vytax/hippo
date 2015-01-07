#include "pdfreply.h"

#include <QImage>
#include <QBuffer>
#include <QtConcurrentRun>

PdfReply::PdfReply(QObject* parent, const QNetworkRequest& request, pdfCache* pdfdoc) :
    QNetworkReply(parent), offset(0), pdf(pdfdoc)
{
    setRequest(request);
    setOperation(QNetworkAccessManager::GetOperation);
    setUrl(request.url());
    hash = url().host();

    QString p = url().path().remove(0, 1);

    bool ok;

    pageNum = p.toInt(&ok);

    if (!ok)
        return;

    Poppler::Document *doc = pdf->getDocument(hash);

    if (pageNum >= doc->numPages())
        return;

    open(ReadOnly | Unbuffered);

    if (hash.isEmpty())
        return;

    watcher = new QFutureWatcher<QByteArray>(this);
    connect(watcher, SIGNAL(finished()), SLOT(loadDone()));
    QFuture<QByteArray> future = QtConcurrent::run<QByteArray>(load, pageNum, doc);
    watcher->setFuture(future);
}

PdfReply::~PdfReply()
{
    watcher->cancel();
    watcher->waitForFinished();
}

qint64 PdfReply::bytesAvailable() const
{
    return QNetworkReply::bytesAvailable() + m_buffer.size() - offset;
}

qint64 PdfReply::readData(char *data, qint64 maxSize)
{
    if (offset < m_buffer.size()) {
        const qint64 size = qMin(maxSize, (qint64)(m_buffer.size()) - offset);
        memcpy(data, m_buffer.constData() + offset, size);
        offset += size;
        return size;
    }
    else
        return -1;
}

void PdfReply::abort()
{
    offset = 0;
}

QByteArray PdfReply::load(int page, Poppler::Document *doc)
{
    QImage img = doc->page(page)->renderToImage(120, 120);
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "JPEG");

    return ba;

}

void PdfReply::loadDone()
{
    offset = 0;
    m_buffer = watcher->result();

    setHeader(QNetworkRequest::ContentTypeHeader, "image/jpeg");
    setHeader(QNetworkRequest::ContentLengthHeader, m_buffer.size());

    emit readyRead();
    emit finished();
}

bool PdfReply::isSequential() const
{
    return true;
}
