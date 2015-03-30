#include "jsbridge.h"
#include "passworddialog.h"
#include "pdfpagedialog.h"
#include "note.h"
#include "sql.h"
#include "edamprotocol.h"
#include "resource.h"
#include <Logger.h>
#include <rc2.h>
#include <zlib.h>

#include <QCryptographicHash>
#include <QFileDialog>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QTimer>
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>
#include <QDrag>
#include <QMimeData>

jsBridge::jsBridge(QObject *parent, pdfCache * pdfc) :
    QObject(parent), pdf(pdfc), webview(NULL)
{
}

QString jsBridge::decrypt(QString data, QString hint)
{
    bool error = false;
    while (true) {
        PasswordDialog pwd(qApp->activeWindow(), false);
        pwd.setHint(hint);
        pwd.setError(error);
        if (pwd.exec() == QDialog::Accepted){

            RC2_KEY key;
            QByteArray b = QByteArray::fromBase64(data.toLatin1());

            QByteArray r = QCryptographicHash::hash(pwd.getPassword().toUtf8(), QCryptographicHash::Md5);
            RC2_set_key(&key, r.size(), (const unsigned char*)r.data(), 64);

            unsigned char * buf2 = new unsigned char[8];

            QByteArray result;

            while (b.size()>0) {
                QByteArray x = b.left(8);
                RC2_ecb_encrypt((const unsigned char*)x.data(),buf2,&key,RC2_DECRYPT);
                result += QByteArray((const char *)buf2, x.size());
                b.remove(0, 8);
            }

            QString crc1 = QString::fromUtf8(result.left(4));
            result.remove(0, 4);

            ulong crc = crc32(0, NULL, 0);
            crc = crc32(crc, (const Bytef *)result.data(), result.size());
            QString crc2 = QString(QByteArray::number(~(uint)crc, 16).left(4));

            if (QString::compare(crc1, crc2, Qt::CaseInsensitive) != 0){
                error = true;
                continue;
            }

            return result;
        }
        else
            return QString();
    }
    return QString();
}

QVariantMap jsBridge::encrypt(QString data)
{
    QVariantMap passJson;
    passJson["ok"] = false;

    PasswordDialog pwd(qApp->activeWindow(), true);
    if (pwd.exec() != QDialog::Accepted)
        return passJson;

    passJson["hint"] = pwd.getHint();

    RC2_KEY key;
    QByteArray r = QCryptographicHash::hash(pwd.getPassword().toUtf8(), QCryptographicHash::Md5);
    RC2_set_key(&key, r.size(), (const unsigned char*)r.data(), 64);

    QByteArray b = data.toUtf8();

    char null_ = 0;
    while (((b.size() + 4) % 8) != 0)
        b.append(null_);

    ulong crc = crc32(0, NULL, 0);
    crc = crc32(crc, (const Bytef *)b.data(), b.size());
    QString crc2 = QString(QByteArray::number(~(uint)crc, 16).left(4)).toUpper();

    b = crc2.toLatin1() + b;

    unsigned char * buf2 = new unsigned char[8];

    QByteArray result;

    while (b.size()>0) {
        QByteArray x = b.left(8);
        RC2_ecb_encrypt((const unsigned char*)x.data(),buf2,&key,RC2_ENCRYPT);
        result += QByteArray((const char *)buf2, 8);
        b.remove(0, 8);
    }

    if (!result.isEmpty()){
        passJson["data"] = QString::fromLatin1(result.toBase64());
        passJson["ok"] = true;
    }

    return passJson;
}

void jsBridge::saveAsResource(QString hash)
{
    Resource *res = Resource::fromHash(hash);
    if (res == NULL)
        return;

    QString mime = res->mimeType();
    QString title = "Save File As";
    QString type = "";
    QString path = QDir::homePath() + QDir::separator() + res->getFileName();
    QByteArray data = res->getData();
    delete res;

    if (mime == "application/pdf") {
        title = "Save PDF document As";
        type = "PDF Documents (*.pdf)";
    } else if (mime == "image/jpeg") {
        title = "Save Image As";
        type = "JPEG Images (*.jpg)";
    } else if (mime == "image/gif") {
        title = "Save Image As";
        type = "GIF Images (*.gif)";
    } else if (mime == "image/png") {
        title = "Save Image As";
        type = "PNG Images (*.png)";
    }
    QString fileName = QFileDialog::getSaveFileName((QWidget*)this->parent(), title, path, type);

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
    f.write(data);
    f.close();
}

void jsBridge::externalOpenResource(QString hash)
{
    Resource *res = Resource::fromHash(hash);
    if (res == NULL)
        return;

    QString mime = res->mimeType();
    QString filename = res->getFileName();
    QByteArray data = res->getData();
    delete res;

    if (filename.isEmpty())
        filename = hash;

    if (mime == "application/pdf") {
        if (!filename.endsWith(".pdf", Qt::CaseInsensitive))
            filename += ".pdf";
    } else if (mime == "image/jpeg") {
        if (!filename.endsWith(".jpg", Qt::CaseInsensitive) && !filename.endsWith(".jpeg", Qt::CaseInsensitive))
            filename += ".jpg";
    } else if (mime == "image/png") {
        if (!filename.endsWith(".png", Qt::CaseInsensitive))
            filename += ".png";
    } else if (mime == "image/gif") {
        if (!filename.endsWith(".gif", Qt::CaseInsensitive))
            filename += ".gif";
    }

    QString tmpl = QDir::tempPath() + QDir::separator() + filename;

    QFile* f = new QFile(tmpl);

    if (!f->open(QIODevice::WriteOnly))
        return;

    f->write(data);
    f->close();
    files.enqueue(f);

    QTimer::singleShot(60000, this, SLOT(removeTmpFile()));
    QDesktopServices::openUrl(QUrl(tmpl));

}

void jsBridge::removeTmpFile()
{
    if (files.isEmpty())
        return;
    QFile *tmpf = files.dequeue();
    tmpf->remove();
    delete tmpf;
}

jsBridge::~jsBridge()
{
    while (!files.isEmpty())
        removeTmpFile();
}

int jsBridge::PDFpageCount(QString hash)
{
    Poppler::Document *doc = pdf->getDocument(hash);

    if (doc == NULL)
        return 0;
    return doc->numPages();
}

int jsBridge::requestPDFpageNumber(int max, int current)
{
    pdfPageDialog dialog;
    dialog.setMaxValue(max);
    dialog.setCurrentPage(current);
    if (dialog.exec() == QDialog::Accepted){
        return dialog.getPageNumber();
    }
    return -1;
}

void jsBridge::debug(QString d) {
    LOG_INFO( d );
}

QString jsBridge::getResourceFileName(QString hash)
{
    Resource *res = Resource::fromHash(hash);
    if (res == NULL)
        return "";

    QString filename = res->getFileName();
    delete res;

    return filename;
}

void jsBridge::dragResource(QString hash) {
    Resource *res = Resource::fromHash(hash);
    if (res == NULL)
        return;

    QString mime = res->mimeType();
    QString fileName = res->getFileName();
    QByteArray data = res->getData();
    delete res;

    if (fileName.isEmpty())
        fileName = hash;

    if (mime == "application/pdf") {
        if (!fileName.endsWith(".pdf", Qt::CaseInsensitive))
            fileName += ".pdf";
    } else if (mime == "image/jpeg") {
        if (!fileName.endsWith(".jpg", Qt::CaseInsensitive) && !fileName.endsWith(".jpeg", Qt::CaseInsensitive))
            fileName += ".jpg";
    } else if (mime == "image/png") {
        if (!fileName.endsWith(".png", Qt::CaseInsensitive))
            fileName += ".png";
    } else if (mime == "image/gif") {
        if (!fileName.endsWith(".gif", Qt::CaseInsensitive))
            fileName += ".gif";
    }

    QString tmpl = QDir::tempPath() + QDir::separator() + fileName;

    QFile* f = new QFile(tmpl);

    if (!f->open(QIODevice::WriteOnly))
        return;

    f->write(data);
    f->close();
    files.enqueue(f);

    QTimer::singleShot(60000, this, SLOT(removeTmpFile()));

    QDrag *drag = new QDrag(new QWidget());
    QMimeData *mimeData = new QMimeData;

    QFileInfo fileInfo(tmpl);
    QUrl url = QUrl::fromLocalFile(fileInfo.absoluteFilePath());
    mimeData->setUrls(QList<QUrl>() << url);

    drag->setMimeData(mimeData);

    QPixmap pix;
    pix.loadFromData(data);
    drag->setPixmap(pix.scaled(128,128, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    drag->exec(Qt::CopyAction | Qt::MoveAction);
}

void jsBridge::deleteResource(QString hash, QString note)
{
    LOG_INFO("hash: '"+ hash + "' note: '" + note + "'");

    QSqlQuery result;
    result.prepare("DELETE FROM resources WHERE bodyHash=:bodyHash AND noteGuid=:noteGuid");
    result.bindValue(":bodyHash", hash);
    result.bindValue(":noteGuid", note);
    result.exec();

    Note::editField(note, Note::T_RECOURCES);

    result.clear();
    result.prepare("SELECT COUNT(*) FROM resources WHERE bodyHash=:bodyHash");
    result.bindValue(":bodyHash", hash);
    result.exec();

    if (result.next() && (result.value(0).toInt() > 0))
        return;

}

QString jsBridge::newNoteGuid()
{
    return newGUID("notes", "guid");
}

bool jsBridge::updateNote(QVariantMap json)
{
    if (json.isEmpty())
        return false;

    QString guid = json["guid"].toString();
    if (guid.isEmpty())
        return false;

    LOG_INFO(guid);

    Note::NoteUpdates updates;

    if (json["contentModified"].toBool()) {
        updates[Note::T_CONTENT] = json["content"];
        qDebug() << json["content"].toString();
    }

    if (json["titleModified"].toBool())
        updates[Note::T_TITLE] =  json["title"];

    if (updates.empty())
        return true;

    updates[Note::T_UPDATED] =  json["updated"];

    Note *note = new Note();
    note->loadFromSQL(guid);
    note->update(updates);
    delete note;

    emit noteUpdated(guid);

    return true;
}

QVariantMap jsBridge::trimWord(QString word) {
    int fromStart, fromEnd;

    for (fromStart = 0; fromStart < word.length(); fromStart++){
        if (word.at(fromStart).isLetter())
            break;
    }
    for (fromEnd = 0; fromEnd < word.length(); fromEnd++){
        if (word.at(word.length() - fromEnd - 1).isLetter())
            break;
    }
    QVariantMap json;
    json["fromStart"] = fromStart;
    json["fromEnd"] = fromEnd;
    return json;
}

QVariantMap jsBridge::saveImageLocally(QString url, QString noteGuid) {

    QVariantMap resJson;
    resJson["ok"] = false;

    QString tmpl = EdamProtocol::GetInstance()->getNetworkManager()->getURLtoFile(url);

    if (!tmpl.isEmpty()){
        Resource res;
        res.setNoteGuid(noteGuid);
        if (res.create(tmpl)) {
            resJson["hash"] = res.getBodyHash();
            resJson["mime"] = res.mimeType();
            resJson["ok"] = true;
        }
    }

    return resJson;
}

void jsBridge::setWebView(QWebView *webView) {
    webview = webView;
}

QString jsBridge::selectedHtml() {
    if (webview == NULL)
        return "";

    return webview->selectedHtml();
}

QVariantMap jsBridge::loadNote(QString guid) {

    LOG_INFO(guid);

    QVariantMap noteJson;
    noteJson["ok"] = false;

    Note *n = Note::fromGUID(guid);
    if (n == NULL)
        return noteJson;

    noteJson["content"] = n->getContent();
    noteJson["guid"] = guid;
    noteJson["title"] = n->getTitle();
    noteJson["active"] = n->getActive();
    noteJson["updated"] = n->getUpdated().toMSecsSinceEpoch();
    noteJson["contentHash"] = n->getContentHash();

    QVariantMap conflict = n->conflict();
    if (!conflict.isEmpty()) {
        noteJson["hasConflict"] = true;
        noteJson["conflict"] = conflict;
    } else
        noteJson["hasConflict"] = false;

    delete n;

    noteJson["ok"] = true;
    return noteJson;
}
