#ifndef OAUTH_H
#define OAUTH_H

#include "netmanager.h"

#include <QDialog>
#include <QUrl>
#include <QWebView>
#include <QProgressBar>

typedef QHash<QString, QString> Arr;

class oauth : public QDialog
{
    Q_OBJECT
public:
    explicit oauth(QWidget *parent = 0);

    QString getParam(QString key);
    
signals:
    void p_oauth_verifier_received();
    void p_finished();
    void p_setup_finished();
    void p_restart();

private:
    void parseReply(QString str);
    Arr data;

    NetDownloadState *get_oauth_token_state;
    NetDownloadState *get_credentials_state;
    QWebView *web;
    QProgressBar *progres;

private slots:
    void parse_outh_token();
    void parse_credentials();
    void urlChange(QUrl url);
    void htmlLoaded();
    void openURL(QUrl url);
    void openNetworkSettings();
    void setup_oauth();
    void changeServer(QString hostName);
};

#endif // OAUTH_H
