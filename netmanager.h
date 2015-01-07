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
    QString getURL(QUrl url);
    QByteArray postData(QUrl url, QByteArray data, bool &ok);
    QNetworkReply::NetworkError getLastError();

private:
    QNetworkAccessManager *nm;
    void saveFile(QNetworkReply *Reply);

    QNetworkReply::NetworkError lastError;
private slots:
    void sslErrorHandler(QNetworkReply* qnr, const QList<QSslError> & sslErrors);
    void replyError( QNetworkReply::NetworkError code );
};

#endif // NETMANAGER_H
