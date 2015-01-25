#include "customnetworkaccessmanager.h"
#include "recourcereply.h"
#include "pdfreply.h"
#include "networkproxyfactory.h"

#include <QNetworkProxy>

#include <QDebug>

CustomNetworkAccessManager::CustomNetworkAccessManager(QNetworkAccessManager *manager, QObject *parent, pdfCache* pdfc) :
    QNetworkAccessManager(parent), pdf(pdfc)
{
    setCache(manager->cache());
    setCookieJar(manager->cookieJar());

    setProxyFactory(NetworkProxyFactory::GetInstance());
}

QNetworkReply* CustomNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *device)
{
    if (req.url().scheme() == "resource")
        return new RecourceReply(this, req);

    if (req.url().scheme() == "pdf")
        return new PdfReply(this, req, pdf);

    return QNetworkAccessManager::createRequest(op, req, device);
}
