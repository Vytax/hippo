#include "netmanager.h"
#include "networkproxyfactory.h"
#include "optionsdialog.h"
#include <QStringList>
#include <QFileDialog>
#include <QtSingleApplication>
#include <QMessageBox>
#include <QDebug>

NetManager::NetManager(QObject *parent):
        QObject(parent)
{
    nm = new QNetworkAccessManager(this);
    nm->setProxyFactory(NetworkProxyFactory::GetInstance());
    connect(nm, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorHandler(QNetworkReply*,QList<QSslError>)));
}

QByteArray NetManager::postData(QUrl url, QByteArray data, bool &ok) {

    QNetworkRequest header;
    header.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/x-thrift") );
    header.setHeader(QNetworkRequest::ContentLengthHeader, QString::number(data.size()));
    header.setUrl(url);

    QEventLoop loop;

    QNetworkReply *reply = nm->post(header, data);
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));

    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        ok = false;
        replyError(reply);
        return QByteArray();
    }

    ok = true;
    return reply->readAll();

}

void NetManager::downloadReply(const QNetworkRequest & request) {

    QEventLoop loop;

    QNetworkReply *reply = nm->get(request);
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() == QNetworkReply::NoError)
        saveFile(reply);
}
QByteArray NetManager::getURL(QUrl url) {

    QNetworkReply *reply= nm->get(QNetworkRequest(url));
    QEventLoop loop;
    connect(reply, SIGNAL(finished()),&loop, SLOT(quit()));
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        replyError(reply);
        return "";
    }

    if (reply->size() > 5000000)
        return "";

    return reply->readAll();
}

QString NetManager::getURLtoFile(QUrl url) {

    QByteArray data = getURL(url);

    QString tmpl = QDir::tempPath() + QDir::separator() + "qEvernote" + QDir::separator() + "tmp.dat";

    QFile f(tmpl);

    if (!f.open(QIODevice::WriteOnly))
        return "";

    f.write(data);
    f.close();

    return tmpl;
}


void NetManager::saveFile(QNetworkReply *Reply) {
    QString urlStr = Reply->url().toString(QUrl::RemoveQuery | QUrl::RemoveFragment | QUrl::StripTrailingSlash);
    QString file = urlStr.split("/", QString::SkipEmptyParts).last();

    QString fileName = QFileDialog::getSaveFileName(qApp->activeWindow(), "Save File As", QDir::homePath() + QDir::separator() + file);

    if (fileName.isNull())
        return;

    QFile f(fileName, this);
    if (!f.open(QIODevice::WriteOnly)){
        QMessageBox msgBox;
        msgBox.setText("An error occurred when writing to the file.");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        return;
    }
    f.write(Reply->readAll());
    f.close();

    qDebug() << fileName;
}

void NetManager::sslErrorHandler(QNetworkReply* qnr, const QList<QSslError> & sslErrors)
{
    for (int i = 0; i < sslErrors.size(); ++i) {
        QSslError e = sslErrors.at(i);

        qDebug() << e.errorString();
    }
    qnr->ignoreSslErrors(sslErrors);
}

void NetManager::replyError(QNetworkReply *reply)
{
    qDebug() << "NetworkError: " << reply->error() << reply->errorString();

    QMessageBox msgBox;
    msgBox.setText(reply->errorString());
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setWindowTitle("Network Error...");
    QAbstractButton * netSettingsB = (QAbstractButton *)msgBox.addButton("Network Settings", QMessageBox::ActionRole);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.exec();

    if (msgBox.clickedButton() == netSettingsB) {
        OptionsDialog dialog(&msgBox);
        dialog.selectTabByName("network");
        dialog.exec();
    }

    return;
}
