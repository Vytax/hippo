#ifndef OAUTH_H
#define OAUTH_H

#include <QDialog>
#include <QUrl>
#include <QWebView>

typedef QHash<QString, QString> Arr;

class oauth : public QDialog
{
    Q_OBJECT
public:
    explicit oauth(QWidget *parent = 0);

    QString getParam(QString key);
    
signals:
    void verifierRetrieved(QString token, QString verifier);

    void p_oauth_token_init_finished();
    void p_oauth_token_finished();
    void p_oauth_verifier_received();
    void p_finished();

private:
    void parseReply(QString str);
    QString queryUrl;
    Arr data;

    QNetworkReply *tmpReply1, *tmpReply2;
    QWebView *web;

private slots:
    void get_oauth_token();
    void parse_outh_token();
    void parse_credentials();
    void urlChange(QUrl url);
};

#endif // OAUTH_H
