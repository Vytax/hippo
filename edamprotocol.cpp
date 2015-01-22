#include "edamprotocol.h"
#include "oauth.h"
#include "qblowfish.h"
#include "networkproxyfactory.h"
#include <QEventLoop>
#include <QDebug>

EdamProtocol* EdamProtocol::m_Instance = NULL;

const QString EdamProtocol::consumerKey = QString("vmickus");
const QString EdamProtocol::consumerSecret = QString("dbf6954858e2cd55");


const QString EdamProtocol::evernoteHost = QString("www.evernote.com");
//const QString EdamProtocol::evernoteHost = QString("sandbox.evernote.com");
const QString EdamProtocol::secret = QString("7e00dc7d49772ca771a844b4561b26a1");

EdamProtocol::EdamProtocol(QObject *parent):
    QObject(parent)
{    
    syncDisabled = false;

    userStoreUri = QUrl(QString("https://%1/edam/user").arg(evernoteHost));

    database = new sql(this);
    database->checkTables();

    NetworkProxyFactory::GetInstance()->loadSettings();
    nm = new NetManager(this);

    timer = new QTimer(this);
    setSyncInterval(sql::readSyncStatus("SyncInterval", 10).toInt());

    s = new Sync(this);
    connect(s, SIGNAL(syncStarted(int)), this, SIGNAL(syncStarted(int)));
    connect(s, SIGNAL(syncProgress(int)), this, SIGNAL(syncProgress(int)));
    connect(s, SIGNAL(syncFinished()), this, SIGNAL(syncFinished()));
    connect(s, SIGNAL(syncRangeChange(int)), this, SIGNAL(syncRangeChange(int)));
    connect(s, SIGNAL(noteGuidChanged(QString,QString)), this, SIGNAL(noteGuidChanged(QString,QString)));
    connect(s, SIGNAL(syncStarted(int)), timer, SLOT(stop()));
    connect(s, SIGNAL(syncFinished()), timer, SLOT(start()));
    connect(timer, SIGNAL(timeout()), this, SLOT(sync()));
}

EdamProtocol* EdamProtocol::GetInstance()
{
    if ( m_Instance == NULL ) {
        m_Instance = new EdamProtocol();
    }
    return m_Instance;
}

void EdamProtocol::deleteInstance()
{
    if ( m_Instance != NULL )
        delete m_Instance;
    m_Instance = NULL;
}

void EdamProtocol::init()
{
    qDebug() << "init()";

    checkVersion();

    if (!vesionAccepted)
        return;

    QBlowfish bf(secret.toLatin1());
    bf.setPaddingEnabled(true);

    QString token_encrypted = sql::readSyncStatus("oauth_token").toString();
    authenticationToken = bf.decrypted(QByteArray::fromHex(token_encrypted.toLatin1()));
    noteStoreUri = sql::readSyncStatus("edam_noteStoreUrl").toUrl();
    expiration = QDateTime::fromMSecsSinceEpoch(sql::readSyncStatus("edam_expires").toLongLong());

    if (!authenticated())
        authenticate();

    if (syncDisabled)
        emit syncFinished();
    else
        sync();
}

QByteArray EdamProtocol::createVersionCheckPost()
{
    TBinaryProtocol *bin = new TBinaryProtocol();
    bin->writeMessageBegin("checkVersion", T_CALL, 0);
    bin->writeString("Pythogsss", 1);
    bin->writeI16(EDAM_VERSION_MAJOR, 2);
    bin->writeI16(EDAM_VERSION_MINOR, 3);
    bin->writeFieldStop();
    QByteArray result = bin->getData();
    delete bin;
    return result;
}

void EdamProtocol::checkVersion()
{    
    vesionAccepted = false;

    bool ok;
    QByteArray result = EdamProtocol::GetInstance()->getNetworkManager()->postData(EdamProtocol::GetInstance()->getUserStoreUri(), createVersionCheckPost(), ok);

    if (!ok) {
       QNetworkReply::NetworkError error = EdamProtocol::GetInstance()->getNetworkManager()->getLastError();
        if (error == QNetworkReply::HostNotFoundError) {
            syncDisabled = true;
            vesionAccepted = true;
        }

        qDebug() << "NET ERROR";
        return;
    }

    TBinaryProtocol *bin = new TBinaryProtocol(result);

    QString name;
    TMessageType messageType;
    qint32 seqid;
    bin->readMessageBegin(name, messageType, seqid);
    if (messageType == T_EXCEPTION){
        qDebug() << "Error:" << "checkVersion failed: unknown result";
    }

    hash data = bin->readField();
    delete bin;

    if (data.contains(0) && data[0].toBool()){
        qDebug() << "vesionAccepted";
        vesionAccepted = true;
    }
}

void EdamProtocol::authenticate()
{
    oauth login;

    if( login.exec() == QDialog::Accepted ) {
        authenticationToken = login.getParam("oauth_token");
        noteStoreUri = QUrl(login.getParam("edam_noteStoreUrl"));

        qlonglong edam_expires = login.getParam("edam_expires").toLongLong();
        expiration = QDateTime::fromMSecsSinceEpoch(edam_expires);

        QBlowfish bf(secret.toLatin1());
        bf.setPaddingEnabled(true);

        sql::updateSyncStatus("oauth_token", bf.encrypted(authenticationToken.toUtf8()).toHex());
        sql::updateSyncStatus("edam_noteStoreUrl", noteStoreUri);
        sql::updateSyncStatus("edam_expires", edam_expires);

    } else {
        emit AuthenticateFailed();
    }
}

void EdamProtocol::sync()
{
    if (authenticated())
        s->sync();
}

QString EdamProtocol::getAuthenticationToken()
{
    return authenticationToken;
}

QUrl EdamProtocol::getNoteStoreUri()
{
    return noteStoreUri;
}

QUrl EdamProtocol::getUserStoreUri()
{
    return userStoreUri;
}

NetManager* EdamProtocol::getNetworkManager()
{
    return nm;
}

sql* EdamProtocol::getDB()
{
    return database;
}

Sync *EdamProtocol::getSyncEngine()
{
    return s;
}

void EdamProtocol::cancelSync()
{
    s->cancelSync();
}

void EdamProtocol::setCNAM(CustomNetworkAccessManager* n)
{
    cnam = n;
}

CustomNetworkAccessManager* EdamProtocol::getCNAM()
{
    return cnam;
}

bool EdamProtocol::authenticated()
{
    if (authenticationToken.isEmpty() || noteStoreUri.isEmpty())
        return false;

    return (QDateTime::currentDateTime() < expiration);
}

void EdamProtocol::setSyncInterval(int min)
{
    timer->setInterval(min * 60 * 1000);
}
