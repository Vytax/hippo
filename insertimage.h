#ifndef INSERTIMAGE_H
#define INSERTIMAGE_H

#include "resource.h"

#include <QDialog>
#include <QNetworkReply>

class Resource;

namespace Ui {
class insertImage;
}

class insertImage : public QDialog
{
    Q_OBJECT
    
public:
    enum img_type {IMG_NONE, IMG_URL, IMG_LOCAL};

    explicit insertImage(QWidget *parent, QString note);
    ~insertImage();
    QString getUrl();
    img_type getType();
    QString getBodyHash();
    QString mimeType();


private slots:
    void validateURL(QString link = QString());
    void urlAccept();
    void selectFile();
    void fileAccept();
    
private:
    Ui::insertImage *ui;
    QStringList alowedSchemas;
    QString url;
    QStringList imageTypes;
    QString noteGuid;
    img_type type;
    QString hash;
    QString mime;

    bool validateURLEnding(QString link);
};

#endif // INSERTIMAGE_H
