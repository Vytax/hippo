#include "edamprotocol.h"
#include "qblowfish.h"
#include "networkproxyfactory.h"
#include "Logger.h"
#include <QEventLoop>
#include <QState>
#include <QFinalState>
#include <QStateMachine>
#include <QMessageBox>

EdamProtocol* EdamProtocol::m_Instance = NULL;

const QString EdamProtocol::consumerKey = QString("vmickus");
const QString EdamProtocol::consumerSecret = QString("dbf6954858e2cd55");


const QString EdamProtocol::evernoteHost = QString(qgetenv("EVERNOTEHOST"));//"app.yinxiang.com");
//const QString EdamProtocol::evernoteHost = QString("sandbox.evernote.com");
const QString EdamProtocol::secret = QString("7e00dc7d49772ca771a844b4561b26a1");

EdamProtocol::EdamProtocol(QObject *parent):
    QObject(parent)
{
    login = NULL;
    syncDisabled = false;

    userStoreUri = QUrl(QString("https://%1/edam/user").arg(evernoteHost));

    database = new sql(this);

    database->checkTables();

    if (!database->test()) {
        QString msg("SQLite database load failed!");

        QMessageBox msgBox;
        msgBox.setText(msg);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();

        LOG_FATAL(msg);
        return;
    }

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
    connect(s, SIGNAL(noteUpdated(QString)), this, SIGNAL(noteUpdated(QString)));
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
        m_Instance->deleteLater();
    m_Instance = NULL;
}

void EdamProtocol::init()
{
    QState *s1 = new QState();
    QState *s2 = new QState();
    QState *s3 = new QState();
    QState *s4 = new QState();
    QFinalState *s5 = new QFinalState();

    s1->addTransition(this, SIGNAL(p_logged_in()), s2);
    s1->addTransition(this, SIGNAL(p_need_login()), s3);
    s3->addTransition(this, SIGNAL(p_authentificated()), s4);
    s4->addTransition(this, SIGNAL(p_logged_in()), s2);
    s2->addTransition(this, SIGNAL(syncFinished()), s5);

    connect(s1, SIGNAL(entered()), this, SLOT(readLoginData()));
    connect(s2, SIGNAL(entered()), this, SLOT(sync()));
    connect(s3, SIGNAL(entered()), this, SLOT(start_authenticate()));
    connect(s4, SIGNAL(entered()), this, SLOT(writeLoginData()));

    QStateMachine *machine = new QStateMachine(this);
    machine->addState(s1);
    machine->addState(s2);
    machine->addState(s3);
    machine->addState(s4);
    machine->addState(s5);

    machine->setInitialState(s1);
    machine->start();

}

void EdamProtocol::readLoginData() {
    QBlowfish bf(secret.toLatin1());
    bf.setPaddingEnabled(true);

    QString token_encrypted = sql::readSyncStatus("oauth_token").toString();
    authenticationToken = bf.decrypted(QByteArray::fromHex(token_encrypted.toLatin1()));
    noteStoreUri = sql::readSyncStatus("edam_noteStoreUrl").toUrl();
    expiration = QDateTime::fromMSecsSinceEpoch(sql::readSyncStatus("edam_expires").toLongLong());

    if (authenticated()) {
        emit p_logged_in();
        LOG_INFO("Authenticating user: " + sql::readSyncStatus("user.username").toString());
    } else
        emit p_need_login();
}

void EdamProtocol::start_authenticate()
{
    login = new oauth();
    connect(login, SIGNAL(finished(int)), this, SIGNAL(p_authentificated()));
    login->show();
}

void EdamProtocol::writeLoginData()
{
    if (login == NULL)
        return;

    if( login->result() == QDialog::Accepted ) {
        authenticationToken = login->getParam("oauth_token");
        noteStoreUri = QUrl(login->getParam("edam_noteStoreUrl"));

        qlonglong edam_expires = login->getParam("edam_expires").toLongLong();
        expiration = QDateTime::fromMSecsSinceEpoch(edam_expires);

        QBlowfish bf(secret.toLatin1());
        bf.setPaddingEnabled(true);

        sql::updateSyncStatus("oauth_token", bf.encrypted(authenticationToken.toUtf8()).toHex());
        sql::updateSyncStatus("edam_noteStoreUrl", noteStoreUri);
        sql::updateSyncStatus("edam_expires", edam_expires);

        emit p_logged_in();

    } else {
        emit AuthenticateFailed();
    }
}

void EdamProtocol::sync()
{
    if (syncDisabled) {
        emit syncFinished();
        return;
    }

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
