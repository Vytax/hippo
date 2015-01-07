#ifndef PDFPAGEDIALOG_H
#define PDFPAGEDIALOG_H

#include <QDialog>
#include <QSpinBox>

class pdfPageDialog : public QDialog
{
    Q_OBJECT
public:
    explicit pdfPageDialog(QWidget *parent = 0);
    void setMaxValue(int value);
    int getPageNumber();
    void setCurrentPage(int value);

private:
    QSpinBox *number;

};

#endif // PDFPAGEDIALOG_H
