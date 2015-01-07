#include "insertimage.h"
#include "edamprotocol.h"
#include "ui_insertimage.h"

#include <QtSingleApplication>
#include <QClipboard>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QFile>
#include <QDir>
#include <QImageReader>
#include <QEventLoop>
#include <QFileDialog>

#include <QDebug>

insertImage::insertImage(QWidget *parent, QString note) :
    QDialog(parent),
    ui(new Ui::insertImage)
{
    ui->setupUi(this);

    alowedSchemas << "http" << "https";
    imageTypes << "jpg" << "jpeg" << "gif" << "png";
    noteGuid = note;
    type = IMG_NONE;

    if (noteGuid.isEmpty())
        return;

    QString clip;
    QUrl u(qApp->clipboard()->text());

    if (u.isValid() && alowedSchemas.contains(u.scheme()))
        clip = u.toString();

    ui->URLImage->setText(clip);

    ui->fileButtons->button(QDialogButtonBox::Ok)->setDisabled(true);
    ui->urlButtons->button(QDialogButtonBox::Ok)->setDisabled(true);

    connect(ui->URLImage, SIGNAL(textChanged(QString)), this, SLOT(validateURL(QString)));
    connect(ui->saveImage, SIGNAL(clicked()), this, SLOT(validateURL()));
    connect(ui->urlButtons, SIGNAL(accepted()), this, SLOT(urlAccept()));
    connect(ui->selectFile, SIGNAL(clicked()), this, SLOT(selectFile()));
    connect(ui->fileButtons, SIGNAL(accepted()), this, SLOT(fileAccept()));

    validateURL();
}

insertImage::~insertImage()
{
    delete ui;
}

void insertImage::validateURL(QString link)
{
    if (link.isEmpty())
        link = ui->URLImage->text();

    QUrl u(link);
    QPushButton *okButton = ui->urlButtons->button(QDialogButtonBox::Ok);

    bool valid = u.isValid() && alowedSchemas.contains(u.scheme()) && !u.host().isEmpty();

    if (valid && !ui->saveImage->isChecked()) {
        valid = validateURLEnding(link);
    }

    if (valid) {
        ui->info->setText("<span style=\"color:green;\">URL is valid!</span>");
        okButton->setDisabled(false);
    } else {
        ui->info->setText("<span style=\"color:red;\">URL is not valid!</span>");
        okButton->setDisabled(true);
    }
}

bool insertImage::validateURLEnding(QString link) {
    QString type;
    foreach (type, imageTypes) {
        if (link.endsWith("." + type, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

void insertImage::urlAccept() {
    if (ui->saveImage->isChecked()) {

        QString tmpl = EdamProtocol::GetInstance()->getNetworkManager()->getURL(ui->URLImage->text());

        if (tmpl.isEmpty())
            return;        

        Resource res;
        res.setNoteGuid(noteGuid);
        if (res.create(tmpl)) {
            type = IMG_LOCAL;
            hash = res.getBodyHash();
            mime = res.mimeType();

            accept();
        }

    } else {
        url = ui->URLImage->text();
        type = IMG_URL;
        accept();
    }
}

QString insertImage::getUrl() {
    return url;
}

void insertImage::selectFile() {
    ui->localImage->clear();
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Image File"),
                                                    QDir::homePath(),
                                                    tr("Images (*.png *.jpg *.jpeg, *.gif)"));

    QPushButton *okButton = ui->fileButtons->button(QDialogButtonBox::Ok);
    okButton->setDisabled(true);

    if (fileName.isEmpty())
        return;

    QImageReader img(fileName);

    if (!img.canRead())
        return;

    if (!imageTypes.contains(QString::fromLatin1(img.format()), Qt::CaseInsensitive))
        return;

    okButton->setDisabled(false);
    ui->localImage->setText(fileName);
}

void insertImage::fileAccept() {
    if (ui->localImage->text().isEmpty())
        return;

    Resource res;
    res.setNoteGuid(noteGuid);
    if (res.create(ui->localImage->text())) {
        type = IMG_LOCAL;
        hash = res.getBodyHash();
        mime = res.mimeType();
        accept();
    }
}

insertImage::img_type insertImage::getType() {
    return type;
}

QString insertImage::getBodyHash() {
    return hash;
}

QString insertImage::mimeType() {
    return mime;
}
