#include "taglabel.h"
#include <QSqlQuery>
#include <QCompleter>
#include <QRegExp>
#include <QRegExpValidator>

#include <QDebug>

TagLabel::TagLabel(QWidget *parent) :
    QWidget(parent)
{
    layout = new QHBoxLayout(this);
    layout->setMargin(0);
    setLayout(layout);

    label = new QPushButton(this);
    layout->addWidget(label);
    label->setFlat(true);
    label->setText("New Tag...");

    QFont font = this->font();
    font.setUnderline(true);
    label->setFont(font);

    edit = new QLineEdit(this);
    edit->hide();
    edit->setFrame(false);
    layout->addWidget(edit);
    layout->addStretch();

    connect(label, SIGNAL(clicked()), label, SLOT(hide()));
    connect(label, SIGNAL(clicked()), edit, SLOT(show()));
    connect(label, SIGNAL(clicked()), edit, SLOT(setFocus()));
    connect(edit, SIGNAL(returnPressed()), this, SLOT(Created()));
    connect(edit, SIGNAL(editingFinished()), this, SLOT(finished()));

    QSqlQuery result;
    result.exec("SELECT name FROM tags");

    QStringList tags;

    while ( result.next() ) {
        tags.append(result.value(0).toString());
    }

    model = new QStringListModel(tags, this);

    QCompleter *completer = new QCompleter(model, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    edit->setCompleter(completer);
}

void TagLabel::Created() {

    QString tagName = edit->text().trimmed();
    if (tagName.isEmpty() || tagName.length()>=100)
        return;

    edit->hide();
    label->show();
    emit tagCreated(tagName);

    QStringList tags = model->stringList();
    if (!tags.contains(tagName, Qt::CaseInsensitive)) {
        tags << tagName;
        model->setStringList(tags);
    }
    edit->clear();
}

void TagLabel::finished() {
    if (edit->text().isEmpty()) {
        edit->hide();
        label->show();
    }
}
