#include "oauth.h"
#include "edamprotocol.h"
#include "networkproxyfactory.h"

#include <QVBoxLayout>
#include <QDebug>
#include <QEventLoop>
#include <QWebFrame>
#include <QTimer>
#include <QState>
#include <QFinalState>
#include <QStateMachine>
#include <QDebug>

#if QT_VERSION >= 0x050000
#include <QUrlQuery>
#endif

oauth::oauth(QWidget *parent) :
    QDialog(parent)
{
    qDebug() << "oauth::oauth";

    setWindowIcon(QIcon(":/img/evernote64.png"));
    setWindowTitle("Please Grant EverClient Access");

    QVBoxLayout* layout = new QVBoxLayout(this);

    web = new QWebView(this);
    layout->addWidget(web);
    web->setZoomFactor(0.9);
    web->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

    qint64 time = QDateTime::currentDateTime().toMSecsSinceEpoch();
    QString nounce = QString::number(qrand());

    data["queryUrl"] = "https://" + EdamProtocol::evernoteHost + "/oauth?oauth_consumer_key=" + EdamProtocol::consumerKey
            + "&oauth_signature=" + EdamProtocol::consumerSecret + "%26&oauth_signature_method=PLAINTEXT&oauth_timestamp="
            + QString::number(time) + "&oauth_nonce=" + nounce;

     QState *s1 = new QState();
     QState *s2 = new QState();
     QState *s3 = new QState();
     QFinalState *s4 = new QFinalState();

     s1->addTransition(this, SIGNAL(p_oauth_token_init_finished()), s2);
     s2->addTransition(this, SIGNAL(p_oauth_verifier_received()), s3);
     s3->addTransition(this, SIGNAL(p_finished()), s4);

     connect(s1, SIGNAL(entered()), this, SLOT(get_oauth_token()));
     connect(s2, SIGNAL(entered()), this, SLOT(parse_outh_token()));
     connect(s3, SIGNAL(entered()), this, SLOT(parse_credentials()));

     QStateMachine *machine = new QStateMachine(this);
     machine->addState(s1);
     machine->addState(s2);
     machine->addState(s3);
     machine->addState(s4);

     connect(machine, SIGNAL(finished()), this, SLOT(accept()));
     machine->setInitialState(s1);
     machine->start();

}

void oauth::get_oauth_token() {
    tmpReply1 = EdamProtocol::GetInstance()->getNetworkManager()->get(QUrl(data["queryUrl"] + "&oauth_callback=confirm_client"));
    connect(tmpReply1, SIGNAL(finished()), this, SIGNAL(p_oauth_token_init_finished()));
}

void oauth::parse_outh_token() {
    if (tmpReply1->error() != QNetworkReply::NoError) {
        reject();
        return;
    }

    QString d = QString::fromLatin1(tmpReply1->readAll());
    parseReply(d);
    if (data.isEmpty() && !data.contains("oauth_token")) {
        web->setHtml(d);
        return;
    }

    connect(web, SIGNAL(urlChanged(QUrl)), this, SLOT(urlChange(QUrl)));

    web->page()->networkAccessManager()->setProxyFactory(NetworkProxyFactory::GetInstance());
    web->load(QUrl("https://" + EdamProtocol::evernoteHost + "/OAuth.action?oauth_token=" + data["oauth_token"]));
}

void oauth::parseReply(QString str) {
    QString chunk;
    foreach (chunk, str.split("&")) {
        QStringList pair = chunk.split("=", QString::SkipEmptyParts);

        if (pair.size() != 2)
            continue;

        data[pair[0]] = QUrl::fromPercentEncoding(pair[1].toLatin1());
    }
}

void oauth::urlChange(QUrl url) {
    if (url.path() != "/confirm_client")
        return;

#if QT_VERSION >= 0x050000
    QUrlQuery urlQuery(url);
    if ((urlQuery.hasQueryItem("oauth_token")) && (urlQuery.hasQueryItem("oauth_verifier"))) {
        data["oauth_token"] = urlQuery.queryItemValue("oauth_token");
        data["oauth_verifier"] = urlQuery.queryItemValue("oauth_verifier");
    }
    else
        reject();
#else
    if ((url.hasQueryItem("oauth_token")) && (url.hasQueryItem("oauth_verifier"))) {
        data["oauth_token"] = url.queryItemValue("oauth_token");
        data["oauth_verifier"] = url.queryItemValue("oauth_verifier");
    }
    else
        reject();
#endif

    tmpReply2 = EdamProtocol::GetInstance()->getNetworkManager()->get(QUrl(data["queryUrl"] + "&oauth_token=" + data["oauth_token"] + "&oauth_verifier=" + data["oauth_verifier"]));
    connect(tmpReply2, SIGNAL(finished()), this, SIGNAL(p_oauth_verifier_received()));
}

void oauth::parse_credentials() {

    if (tmpReply2->error() != QNetworkReply::NoError) {
        reject();
        return;
    }

    QString d = QString::fromLatin1(tmpReply2->readAll());
    parseReply(d);

    if (!data.isEmpty())
        emit p_finished();
    else
        reject();
}

QString oauth::getParam(QString key) {
    return data[key];
}
