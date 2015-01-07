#include "customwebview.h"
#include "mime.h"
#include "resource.h"
#include "customwebpage.h"

#include <QDebug>
#include <QWebFrame>
#include <QtSingleApplication>
#include <QDir>
#include <QWebSecurityOrigin>
#include <QMimeData>

CustomWebView::CustomWebView(QWidget *parent) :
    QWebView(parent)
{
    fileTypes << "image/gif" << "image/jpeg" << "image/png" << "application/pdf";

    setPage(new CustomWebPage(this));
    page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    page()->settings()->globalSettings()->setFontFamily(QWebSettings::StandardFont, qApp->font().family());
    page()->settings()->globalSettings()->setFontSize(QWebSettings::MinimumFontSize, qApp->font().pointSize());
    page()->settings()->globalSettings()->enablePersistentStorage(QDir::tempPath() + QDir::separator() + "qEvernote" + QDir::separator() + "notes" + QDir::separator());
    page()->settings()->globalSettings()->setAttribute(QWebSettings::LocalStorageEnabled , true);
    page()->settings()->globalSettings()->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls , true);
    page()->settings()->globalSettings()->setAttribute(QWebSettings::PluginsEnabled , false);
    page()->mainFrame()->securityOrigin().addLocalScheme("note");
}

void CustomWebView::dragMoveEvent(QDragMoveEvent *event) {

    if (JS("editMode").toBool()) {
        if (event->mimeData()->hasUrls()) {
            qDebug() << event->mimeData()->urls();
            if (event->mimeData()->urls().size() == 1) {
                QUrl url = event->mimeData()->urls().at(0);
                if (url.isLocalFile()) {
                    QString mime = Mime::GetInstance()->getMime(url.toLocalFile());
                    if (fileTypes.contains(mime, Qt::CaseInsensitive)) {
                        event->accept();
                        return;
                    }
                }
            }
        }
    }

    if (event->mimeData()->hasFormat("qevernote/tag")) {
        event->accept();
        return;
    }

    event->ignore();
}

void CustomWebView::dropEvent(QDropEvent * event) {
    if (JS("editMode").toBool()) {
        if (event->mimeData()->hasUrls()) {
            qDebug() << event->mimeData()->urls();
            if (event->mimeData()->urls().size() == 1) {
                QUrl url = event->mimeData()->urls().at(0);
                if (url.isLocalFile()) {
                    QString fileName = url.toLocalFile();
                    QString mime = Mime::GetInstance()->getMime(fileName);
                    if (fileTypes.contains(mime, Qt::CaseInsensitive)) {
                        emit fileInserted(fileName);
                    }
                }
            }
        }
    }

    if (event->mimeData()->hasFormat("qevernote/tag")) {
        QString data = QString::fromLatin1(event->mimeData()->data("qevernote/tag"));
        emit tagUpdated(data, true);
    }

    event->ignore();
}

QVariant CustomWebView::JS(QString command) {
    return page()->mainFrame()->evaluateJavaScript(command);
}

QString CustomWebView::getCurrentNoteGuid() {
    QString guid = JS("guid").toString();
    if (guid != "main")
        return guid;
    return "";
}
