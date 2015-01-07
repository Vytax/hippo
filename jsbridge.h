#ifndef JSBRIDGE_H
#define JSBRIDGE_H

#include "pdfcache.h"

#include <QObject>
#include <QFile>
#include <QQueue>
#include <QWebView>

class jsBridge : public QObject
{
    Q_OBJECT
public:
    jsBridge(QObject *parent, pdfCache * pdfc);
    ~jsBridge();
    void setWebView(QWebView *webView);

    Q_INVOKABLE QString decrypt(QString data, QString hint);
    Q_INVOKABLE QVariantMap encrypt(QString data);
    Q_INVOKABLE QVariantMap saveImageLocally(QString url, QString noteGuid);
    Q_INVOKABLE QVariantMap trimWord(QString word);
    Q_INVOKABLE QString selectedHtml();
    Q_INVOKABLE QVariantMap loadNote(QString guid);

public slots:
    void saveAsResource(QString hash);
    void externalOpenResource(QString hash);

    void debug(QString d);
    int PDFpageCount(QString hash);
    int requestPDFpageNumber(int max, int current);
    QString getResourceFileName(QString hash);
    void deleteResource(QString hash, QString note);
    QString newNoteGuid();
    bool updateNote(QVariantMap json);


signals:
    void hintMessage (const QString & message, int timeout);
    void noteChanged(QString guid);
    void editingStarted(bool started);
    void noteSelectionChanged(bool selected);
    void activeNoteSelectionChanged(bool selected);
    void titleUpdated(QString guid, QString title);
    void showConflict();
    void conflictAdded(qint64 date, QString hash, bool enabled);
    void encryptText();
    void insertToDo();
    void titleChanged(QString title, QString guid);

private slots:
    void removeTmpFile();

private:
    QQueue<QFile *> files;
    pdfCache * pdf;
    QWebView *webview;

};

#endif // JSBRIDGE_H
