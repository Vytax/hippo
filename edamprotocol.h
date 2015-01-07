#ifndef EDAMPROTOCOL_H
#define EDAMPROTOCOL_H

#include "tbinaryprotocol.h"
#include "notebook.h"
#include "netmanager.h"
#include "sql.h"
#include "sync.h"
#include "customnetworkaccessmanager.h"

#include <QObject>
#include <QDateTime>

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
    void checkVersion();
    void authenticate();
    void setCNAM(CustomNetworkAccessManager* n);

    QString getAuthenticationToken();
    QUrl getNoteStoreUri();
    QUrl getUserStoreUri();
    NetManager *getNetworkManager();
    sql *getDB();
    Sync *getSyncEngine();
    CustomNetworkAccessManager *getCNAM();
    bool authenticated();

    static const QString consumerKey;
    static const QString consumerSecret;
    static const QString evernoteHost;
    static const QString secret;

public slots:
    void cancelSync();
    void sync();

private:
    QUrl userStoreUri;
    QUrl noteStoreUri;
    QString authenticationToken;
    QDateTime expiration;
    bool vesionAccepted;
    bool syncDisabled;

    NetManager *nm;
    sql *database;
    Sync *s;
    CustomNetworkAccessManager* cnam;

    QByteArray createVersionCheckPost();

signals:
    void AuthenticateFailed();
    void syncStarted(int count);
    void syncProgress(int value);
    void syncFinished();
    void syncRangeChange(int max);
    void noteGuidChanged(QString oldGuid, QString newGuid);

};

#endif // EDAMPROTOCOL_H
