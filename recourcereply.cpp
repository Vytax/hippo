#include "recourcereply.h"
#include "resource.h"

#include <QMetaObject>

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

    Resource *res = Resource::fromHash(hash);
    if (res != NULL) {
        offset = 0;
        m_buffer = res->getData();
        setHeader(QNetworkRequest::ContentTypeHeader, res->mimeType());
        setHeader(QNetworkRequest::ContentLengthHeader, m_buffer.size());
    }
    delete res;
    QMetaObject::invokeMethod(this, "loadDone", Qt::QueuedConnection);
}

RecourceReply::~RecourceReply()
{
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

void RecourceReply::loadDone()
{
    emit readyRead();
    emit finished();
}

bool RecourceReply::isSequential() const
{
    return true;
}

