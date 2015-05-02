#ifndef NOTEINFODIALOG_H
#define NOTEINFODIALOG_H

#include "note.h"

#include <QDialog>
#include <QLineEdit>
#include <QDateTimeEdit>

class NoteInfoDialog : public QDialog
{
    Q_OBJECT
public:
    NoteInfoDialog(QWidget *parent, QString guid);
    ~NoteInfoDialog();

signals:

private slots:
    void save();

private:
    Note *note;

    QLineEdit *url;
    QLineEdit *author;
    QDateTimeEdit *reminderEdit;

};

#endif // NOTEINFODIALOG_H
