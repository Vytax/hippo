#ifndef NETMANAGER_H
#define NETMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslError>
#include <QUrl>

class NetManager : public QObject
{
    Q_OBJECT
public:
    NetManager(QObject *parent = 0);
    void downloadReply(const QNetworkRequest & request);
    QByteArray getURL(QUrl url);
    QString getURLtoFile(QUrl url);
    QByteArray postData(QUrl url, QByteArray data, bool &ok);

private:
    QNetworkAccessManager *nm;
    void saveFile(QNetworkReply *Reply);

private slots:
    void sslErrorHandler(QNetworkReply* qnr, const QList<QSslError> & sslErrors);
    void replyError(QNetworkReply *reply );
};

#endif // NETMANAGER_H
