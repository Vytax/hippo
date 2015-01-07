#ifndef TAGLABEL_H
#define TAGLABEL_H

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QStringListModel>

class TagLabel : public QWidget
{
    Q_OBJECT
public:
    explicit TagLabel(QWidget *parent = 0);

signals:
    void tagCreated(QString tag);

private slots:
    void Created();
    void finished();

private:
    QPushButton *label;
    QHBoxLayout *layout;
    QLineEdit *edit;
    QStringListModel *model;

};

#endif // TAGLABEL_H
