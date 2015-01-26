#include "oauth.h"
#include "edamprotocol.h"
#include "networkproxyfactory.h"

#include <QWebView>
#include <QVBoxLayout>
#include <QDebug>
#include <QEventLoop>
#include <QWebFrame>
#include <QTimer>

#if QT_VERSION >= 0x050000
#include <QUrlQuery>
#endif

oauth::oauth(QWidget *parent) :
    QDialog(parent)
{
    setWindowIcon(QIcon(":/img/evernote64.png"));
    setWindowTitle("Please Grant EverClient Access");

    QVBoxLayout* layout = new QVBoxLayout(this);

    QWebView *web = new QWebView(this);
    layout->addWidget(web);
    web->setZoomFactor(0.9);
    web->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

    qint64 time = QDateTime::currentDateTime().toMSecsSinceEpoch();
    QString nounce = QString::number(qrand());

    queryUrl = "https://" + EdamProtocol::evernoteHost + "/oauth?oauth_consumer_key=" + EdamProtocol::consumerKey
            + "&oauth_signature=" + EdamProtocol::consumerSecret + "%26&oauth_signature_method=PLAINTEXT&oauth_timestamp="
            + QString::number(time) + "&oauth_nonce=" + nounce;

    QString tmpreply = QString::fromLatin1(EdamProtocol::GetInstance()->getNetworkManager()->getURL(QUrl(queryUrl + "&oauth_callback=confirm_client")));

    Arr d = parseReply(tmpreply);
    if (d.isEmpty() && !d.contains("oauth_token")) {
        qDebug() << "Klaida!" << d;
        web->setHtml(tmpreply);
        QTimer::singleShot(100, this, SLOT(reject()));
        return;
    }

    connect(web, SIGNAL(urlChanged(QUrl)), this, SLOT(urlChange(QUrl)));
    connect(this, SIGNAL(verifierRetrieved(QString,QString)), this, SLOT(getCredentials(QString,QString)));

    web->page()->networkAccessManager()->setProxyFactory(NetworkProxyFactory::GetInstance());
    web->load(QUrl("https://" + EdamProtocol::evernoteHost + "/OAuth.action?oauth_token=" + d["oauth_token"]));

    setLayout(layout);
}

Arr oauth::parseReply(QString data) {
    Arr result;

    QString chunk;
    foreach (chunk, data.split("&")) {
        QStringList pair = chunk.split("=", QString::SkipEmptyParts);

        if (pair.size() != 2)
            continue;

        result[pair[0]] = QUrl::fromPercentEncoding(pair[1].toLatin1());
    }

    return result;
}

void oauth::urlChange(QUrl url) {
    if (url.path() != "/confirm_client")
        return;

#if QT_VERSION >= 0x050000
    QUrlQuery urlQuery(url);
    if ((urlQuery.hasQueryItem("oauth_token")) && (urlQuery.hasQueryItem("oauth_verifier")))
        emit verifierRetrieved(urlQuery.queryItemValue("oauth_token"), urlQuery.queryItemValue("oauth_verifier"));
    else
        reject();
#else
    if ((url.hasQueryItem("oauth_token")) && (url.hasQueryItem("oauth_verifier")))
        emit verifierRetrieved(url.queryItemValue("oauth_token"), url.queryItemValue("oauth_verifier"));
    else
        reject();
#endif
}

void oauth::getCredentials(QString token, QString verifier) {

    QString tmpreply = QString::fromLatin1(EdamProtocol::GetInstance()->getNetworkManager()->getURL(QUrl(queryUrl + "&oauth_token=" + token + "&oauth_verifier=" + verifier)));
    data = parseReply(tmpreply);

     qDebug() << data;

    if (!data.isEmpty())
        accept();
    else
        reject();
}

QString oauth::getParam(QString key) {
    return data[key];
}
