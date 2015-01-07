#include "recourcereply.h"
#include "resource.h"

#include <QtConcurrentRun>

#include <QDebug>

RecourceReply::RecourceReply(QObject* parent, const QNetworkRequest& request) :
    QNetworkReply(parent), offset(0)
{
    setRequest(request);
    setOperation(QNetworkAccessManager::GetOperation);
    setUrl(request.url());
    hash = url().host();
    open(ReadOnly | Unbuffered);

    if (hash.isEmpty())
        return;

    watcher = new QFutureWatcher<resource>(this);
    connect(watcher, SIGNAL(finished()), SLOT(loadDone()));
    QFuture<resource> future = QtConcurrent::run<resource>(load, hash);
    watcher->setFuture(future);

}

RecourceReply::~RecourceReply()
{
    watcher->cancel();
    watcher->waitForFinished();
}

qint64 RecourceReply::bytesAvailable() const
{
    return QNetworkReply::bytesAvailable() + m_buffer.size() - offset;
}

qint64 RecourceReply::readData(char *data, qint64 maxSize)
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

void RecourceReply::abort()
{
    offset = 0;
}

resource RecourceReply::load(const QString hash)
{
    resource r;

    Resource *res = Resource::fromHash(hash);

    if (res != NULL) {
        r.mime = res->mimeType();
        r.data = res->getData();
        delete res;
    }
    return r;
}

void RecourceReply::loadDone()
{
    offset = 0;
    m_buffer = watcher->result().data;

    setHeader(QNetworkRequest::ContentTypeHeader, watcher->result().mime);
    setHeader(QNetworkRequest::ContentLengthHeader, m_buffer.size());

    emit readyRead();
    emit finished();
}

bool RecourceReply::isSequential() const
{
    return true;
}

