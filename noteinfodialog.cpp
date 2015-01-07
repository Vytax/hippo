#include "noteinfodialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QDialogButtonBox>


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
    layout->addWidget(title, 0, 0, 1, 2);


    layout->addWidget(new QLabel("<b>Overview</b>", this), 2, 0, 1, 2);

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

    QLabel* created = new QLabel(this);
    created->setText(note->getCreated().toString("dddd, MMMM d yyyy, h:mm AP"));
    layout->addWidget(created, 3, 1);

    QLabel* updated = new QLabel(this);
    updated->setText(note->getUpdated().toString("dddd, MMMM d yyyy, h:mm AP"));
    layout->addWidget(updated, 4, 1);

    QLabel* size = new QLabel(this);
    size->setText(QString("%1 bytes").arg(note->getSize()));
    size->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Minimum);
    layout->addWidget(size, 5, 1);

    url = new QLineEdit(this);
    url->setPlaceholderText("Set a URL...");
    url->setText(note->getSourceURL());
    url->setMaxLength(4096);
    layout->addWidget(url, 6, 0, 1, 2);

    author = new QLineEdit(this);
    author->setPlaceholderText("Set an Author...");
    author->setText(note->getAuthor());
    author->setMaxLength(4096);
    layout->addWidget(author, 7, 0, 1, 2);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save, Qt::Horizontal, this);
    connect(buttons, SIGNAL(accepted()), this, SLOT(save()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttons, 8, 0, 1, 2);

    setLayout(layout);
    setMinimumWidth(400);

}

NoteInfoDialog::~NoteInfoDialog() {
    delete note;
}

void NoteInfoDialog::save() {

    note->updateSourceURL(url->text());
    note->updateAuthor(author->text());

    accept();
}
