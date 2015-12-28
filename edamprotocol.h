#ifndef EDAMPROTOCOL_H
#define EDAMPROTOCOL_H

#include "tbinaryprotocol.h"
#include "notebook.h"
#include "netmanager.h"
#include "sql.h"
#include "sync.h"
#include "customnetworkaccessmanager.h"
#include "oauth.h"

#include <QObject>
#include <QDateTime>
#include <QTimer>

static const qint16 EDAM_VERSION_MAJOR = 1;
static const qint16 EDAM_VERSION_MINOR = 17;

class Sync;

class EdamProtocol : public QObject
{
    Q_OBJECT

public:
    EdamProtocol(QObject *parent = 0);

    static EdamProtocol* m_Instance;
    static EdamProtocol* GetInstance();
    static void deleteInstance();

    void init();    
    void setCNAM(CustomNetworkAccessManager* n);

    QString getAuthenticationToken();
    QUrl getNoteStoreUri();
    QUrl getUserStoreUri();
    NetManager *getNetworkManager();
    sql *getDB();
    Sync *getSyncEngine();
    CustomNetworkAccessManager *getCNAM();
    bool authenticated();
    void setSyncInterval(int min);
    static QString evernoteHost();

    static const QString consumerKey;
    static const QString consumerSecret;
    static const QString secret;

public slots:
    void cancelSync();
    void sync();

private slots:
    void readLoginData();
    void start_authenticate();
    void writeLoginData();

private:
    QUrl userStoreUri;
    QUrl noteStoreUri;
    QString authenticationToken;
    QDateTime expiration;
    bool syncDisabled;

    NetManager *nm;
    sql *database;
    Sync *s;
    CustomNetworkAccessManager* cnam;
    QTimer *timer;
    oauth *login;

signals:
    void AuthenticateFailed();
    void syncStarted(int count);
    void syncProgress(int value);
    void syncFinished();
    void syncRangeChange(int max);
    void noteGuidChanged(QString oldGuid, QString newGuid);
    void noteUpdated(QString guid);

    void p_need_login();
    void p_logged_in();
    void p_authentificate_started();
    void p_authentificated();

};

#endif // EDAMPROTOCOL_H
