#include "pdfpagedialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

pdfPageDialog::pdfPageDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Show Page");

    QVBoxLayout* layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("Enter page number:", this));

    number = new QSpinBox(this);
    number->setMinimum(1);
    layout->addWidget(number);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttons);

    setLayout(layout);
}

void pdfPageDialog::setMaxValue(int value)
{
    number->setMaximum(value);
}

int pdfPageDialog::getPageNumber()
{
    return number->value();
}

void pdfPageDialog::setCurrentPage(int value)
{
    number->setValue(value);
}
