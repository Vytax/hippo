#include "noteinfodialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>


NoteInfoDialog::NoteInfoDialog(QWidget *parent, QString guid) :
    QDialog(parent)
{
    if (guid.isEmpty()) {
        reject();
        return;
    }

    note = Note::fromGUID(guid);
    if (note == NULL) {
        reject();
        return;
    }

    setWindowTitle("Note Info");

    QGridLayout* layout = new QGridLayout(this);

    QLabel* title = new QLabel(note->getTitle(), this);
    title->setAlignment(Qt::AlignCenter);
    QFont mainTitleFont, titleFont = title->font();
    mainTitleFont.setBold(true);
    mainTitleFont.setPointSize(15);
    title->setFont(mainTitleFont);
    layout->addWidget(title, 0, 0, 1, 4);


    layout->addWidget(new QLabel("<b>Overview</b>", this), 2, 0, 1, 4);

    titleFont.setCapitalization(QFont::AllUppercase);
    titleFont.setFamily("gotham,helvetica,arial,sans-serif");
    titleFont.setLetterSpacing(QFont::PercentageSpacing, 102);
    titleFont.setWeight(100);
    QLabel* createdTitle = new QLabel("Created:", this);
    createdTitle->setFont(titleFont);

    layout->addWidget(createdTitle, 3, 0);
    QLabel* updatedTitle = new QLabel("Updated:", this);
    updatedTitle->setFont(titleFont);
    layout->addWidget(updatedTitle, 4, 0);
    QLabel* sizeTitle = new QLabel("Size:", this);
    sizeTitle->setFont(titleFont);
    layout->addWidget(sizeTitle, 5, 0);
    QLabel* reminderTitle = new QLabel("Reminder:", this);
    reminderTitle->setFont(titleFont);
    layout->addWidget(reminderTitle, 6, 0);

    QLabel* created = new QLabel(this);
    created->setText(note->getCreated().toString("dddd, MMMM d yyyy, h:mm AP"));
    layout->addWidget(created, 3, 1, 1, 3);

    QLabel* updated = new QLabel(this);
    updated->setText(note->getUpdated().toString("dddd, MMMM d yyyy, h:mm AP"));
    layout->addWidget(updated, 4, 1, 1, 3);

    QLabel* size = new QLabel(this);
    size->setText(QString("%1 bytes").arg(note->getSize()));
    size->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Minimum);
    layout->addWidget(size, 5, 1, 1, 3);

    reminderEdit = new QDateTimeEdit(this);
    reminderEdit->setDisplayFormat("dddd, MMMM d yyyy, h:mm AP");
    reminderEdit->setCalendarPopup(true);
    reminderEdit->setMinimumDateTime(QDateTime::currentDateTime());
    reminderEdit->setHidden(true);
    layout->addWidget(reminderEdit, 6, 1);

    QPushButton *deleteReminder = new QPushButton(this);
    deleteReminder->setIcon(QIcon::fromTheme("edit-delete"));
    deleteReminder->setHidden(true);
    layout->addWidget(deleteReminder, 6, 2);
    connect(deleteReminder, SIGNAL(clicked()), reminderEdit, SLOT(hide()));
    connect(deleteReminder, SIGNAL(clicked()), deleteReminder, SLOT(hide()));

    QPushButton *setReminder = new QPushButton(this);
    setReminder->setIcon(QIcon(":/img/reminder.png"));
    layout->addWidget(setReminder, 6, 3);
    connect(setReminder, SIGNAL(clicked()), reminderEdit, SLOT(show()));
    connect(setReminder, SIGNAL(clicked()), deleteReminder, SLOT(show()));

    QDateTime reminderTime = note->getReminderOrder();
    if (!reminderTime.isNull()) {
        reminderEdit->setHidden(false);
        reminderEdit->setDateTime(reminderTime);

        deleteReminder->setHidden(false);
    }

    url = new QLineEdit(this);
    url->setPlaceholderText("Set a URL...");
    url->setText(note->getSourceURL());
    url->setMaxLength(4096);
    layout->addWidget(url, 7, 0, 1, 4);

    author = new QLineEdit(this);
    author->setPlaceholderText("Set an Author...");
    author->setText(note->getAuthor());
    author->setMaxLength(4096);
    layout->addWidget(author, 8, 0, 1, 4);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save, Qt::Horizontal, this);
    connect(buttons, SIGNAL(accepted()), this, SLOT(save()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttons, 9, 0, 1, 4);

    setLayout(layout);
    setMinimumWidth(400);

}

NoteInfoDialog::~NoteInfoDialog() {
    delete note;
}

void NoteInfoDialog::save() {

    note->updateSourceURL(url->text());
    note->updateAuthor(author->text());

    if (reminderEdit->isVisible())
        note->updateReminderOrder(reminderEdit->dateTime());
    else
        note->updateReminderOrder();

    accept();
}
