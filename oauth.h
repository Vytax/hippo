#ifndef OAUTH_H
#define OAUTH_H

#include <QDialog>
#include <QUrl>
#include <QNetworkAccessManager>

typedef QHash<QString, QString> Arr;

class oauth : public QDialog
{
    Q_OBJECT
public:
    explicit oauth(QWidget *parent = 0);

    bool prepare();
    QString getParam(QString key);
    
signals:
    void verifierRetrieved(QString token, QString verifier);
public slots:
    void urlChange(QUrl url);
    void getCredentials(QString token, QString verifier);

private:
    Arr parseReply(QString data);
    QNetworkAccessManager *manager;
    QString queryUrl;
    Arr data;

    
};

#endif // OAUTH_H
