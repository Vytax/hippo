#ifndef CUSTOMNETWORKACCESSMANAGER_H
#define CUSTOMNETWORKACCESSMANAGER_H

#include "pdfcache.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>

class CustomNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    CustomNetworkAccessManager(QNetworkAccessManager *oldManager, QObject *parent, pdfCache* pdfc);

public slots:

protected:
    QNetworkReply* createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *device);

private:
    pdfCache* pdf;

};

#endif // CUSTOMNETWORKACCESSMANAGER_H
