#ifndef NETMANAGER_H
#define NETMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslError>
#include <QUrl>
#include <QState>
#include <QFinalState>

class NetDownloadState : public QState
{
    Q_OBJECT
public:
    NetDownloadState(QState *parent = 0);
    void get(QUrl url);
    void post(QUrl url, QByteArray data);

    QByteArray data();
    QNetworkReply::NetworkError error();

private slots:
    void request();

private:
    QState *requestState;
    QFinalState *replyState;

    QNetworkReply *reply;

    QNetworkRequest header;
    QByteArray m_data;
};

class NetManager : public QObject
{
    Q_OBJECT
public:
    NetManager(QObject *parent = 0);
    void downloadReply(const QNetworkRequest & request);
    QByteArray getURL(QUrl url);
    QString getURLtoFile(QUrl url);
    QByteArray postData(QUrl url, QByteArray data, bool &ok);

    QNetworkAccessManager *nam();

private:
    QNetworkAccessManager *nm;
    void saveFile(QNetworkReply *Reply);

private slots:
    void sslErrorHandler(QNetworkReply* qnr, const QList<QSslError> & sslErrors);
    void replyError(QNetworkReply *reply);
    void checkReply(QNetworkReply *reply);
};

#endif // NETMANAGER_H
