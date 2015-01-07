#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QVariant>
#include <QFont>
#include <QActionGroup>
#include <QWebElement>
#include "edamprotocol.h"
#include "treeitem.h"
#include "jsbridge.h"
#include "enml2.h"
#include "taglabel.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    void showWindow();
    void saveSelectionState(QString notebook = QString());
    void loadSelectionState(bool selectNote = false);    
    void updateTag(QString noteGuid, QString guid, bool checked);
    void clearConflictBar();
    QString getCurrentNoteGuid();
    QVariant JS(QString command);
    QStringList allClasses(QWebElement element);
    QString exportToHtml(QString dir);
    QActionGroup *tagsActions;
    QActionGroup *conflictsGroup;


    bool loaded;
    QSystemTrayIcon* trayIcon;
    jsBridge *jsB;
    enml2 *enmlWritter;
    bool editingEnabled;
    TagLabel *newTag;

private slots:
    void closeWindow();
    void authentificationFailed();
    void syncStarted(int count);
    void syncFinished();
    void switchNotebook();
    void switchTag();
    void switchNote();
    void loadAboutInfo();
    void trayIconClicked(QSystemTrayIcon::ActivationReason reason);
    void addJSObject();
    void newNote();
    void noteTitleChange(QString newTitle);
    void sync();
    void moveNote(QString note, QString notebook);
    void restoreNote(QString note, QString notebook = QString());
    void openURL(QUrl url);
    void setEditable(bool enabled);
    void updateEditButtonsState();
    void insertHorizontalLine();
    void updateSelectionButtonsState();
    void insertUrl();
    void deleteNote(QString note = QString());
    void changeFont(QString font);
    void changeFontSize(QString fontSize);
    void insertImg();
    void changeTab(int index);
    void linkHovered(QString link, QString title, QString textContent);
    void showUserInfo();
    void downloadRequested(const QNetworkRequest & request);    
    void changeNoteGuid(QString oldGuid, QString newGuid);
    void editorContextMenuRequested ( const QPoint & pos );
    void insertFile();
    void insertFile(QString fileName);
    void updateNoteTitle(QString guid, QString title);
    void replaceWord( QAction * action );
    void updateTagsToolBar(QString noteGuid = QString());
    void tagClicked(QAction * action);
    void createTag(QString tag);
    void addTag(QString noteGuid, QString tagGuid);
    void updateTag(QString guid, bool checked);
    void updateCurrentNoteName();
    void addConflict(qint64 date, QString hash, bool enabled);
    void changeNoteVersion(QAction * action);
    void keepThisVersion();
    void showNoteInfo();
    void exportNote();
    void print();

    void test();

signals:
    void editButtonsStateChanged(bool enabled);
    void selectionButtonsStateChanged(bool enabled);
    void updateFont(QFont font);
    void updateFontSize(int size);
    void titleChanged(QString title, QString guid);

protected:
    void closeEvent(QCloseEvent *event);
};

#endif // MAINWINDOW_H
