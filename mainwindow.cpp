#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "sql.h"
#include "customnetworkaccessmanager.h"
#include "pdfcache.h"
#include "note.h"
#include "urldialog.h"
#include "insertimage.h"
#include "listitem.h"
#include "userinfo.h"
#include "oauth.h"
#include "3rdparty/spellcheck/speller.h"
#include "dbspellsettings.h"
#include "tag.h"
#include "noteinfodialog.h"
#include "resource.h"
#include "optionsdialog.h"
#include "networkproxyfactory.h"

#include <QMessageBox>
#include <QtSingleApplication>
#include <QProgressDialog>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QWebFrame>
#include <QDir>
#include <QDateTime>
#include <QDesktopServices>
#include <QWebHistory>
#include <QMenu>
#include <QByteArray>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QComboBox>
#include <QFileDialog>
#include <QTimer>
#include <QPrinter>
#include <QPrintDialog>

#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#endif

#include <QDebug>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    EdamProtocol *edam = EdamProtocol::GetInstance();

    ui->setupUi(this);

    pdfCache * pdf = new pdfCache(this);

    CustomNetworkAccessManager *nm = new CustomNetworkAccessManager(ui->editor->page()->networkAccessManager(), this, pdf);
    edam->setCNAM(nm);
    ui->editor->page()->setNetworkAccessManager(nm);

    Speller::setSettings(new DBSpellSettings(this, ui->editor));

    ui->editor->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->editor->load(QUrl("qrc:///html/noteajax.html"));

    jsB = new jsBridge(this, pdf);
    jsB->setWebView(ui->editor);
    enmlWritter = new enml2(this);

    connect(ui->editor->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(addJSObject()));

    loaded = false;
    editingEnabled = false;


    tagsActions = new QActionGroup(this);
    newTag = new TagLabel(this);
    newTag->hide();
    ui->tagsBar->addWidget(newTag);

    appIcon = QIcon(":img/hippo64.png");

    trayIcon = NULL;

    bool sysTrayEnabled = sql::readSyncStatus("systemTray", true).toBool();

    if (QSystemTrayIcon::isSystemTrayAvailable() && sysTrayEnabled)
        enableSystemTrayIcon(true);

    connect(edam, SIGNAL(AuthenticateFailed()), this, SLOT(authentificationFailed()));
    connect(edam, SIGNAL(syncFinished()), this, SLOT(syncFinished()));
    connect(edam, SIGNAL(syncStarted(int)), this, SLOT(syncStarted(int)));
    connect(edam, SIGNAL(noteGuidChanged(QString,QString)), this, SLOT(changeNoteGuid(QString,QString)));
    connect(ui->notebooks, SIGNAL(itemSelectionChanged()), this, SLOT(switchNotebook()));
    connect(ui->tags, SIGNAL(itemSelectionChanged()), this, SLOT(switchTag()));
    connect(ui->tags, SIGNAL(tagAdded(QString,QString)), this, SLOT(addTag(QString,QString)));
    connect(ui->tags, SIGNAL(tagsUpdated()), this, SLOT(updateTagsToolBar()));
    connect(ui->tags, SIGNAL(tagsUpdated()), this, SLOT(switchTag()));
    connect(ui->NotesList, SIGNAL(noteSwitched()), this, SLOT(switchNote()));
    connect(ui->action_Abaut, SIGNAL(triggered()), this, SLOT(loadAboutInfo()));
    connect(ui->actionClose, SIGNAL(triggered()), this, SLOT(closeWindow()));
    connect(ui->actionNew_Note, SIGNAL(triggered()), this, SLOT(newNote()));
    connect(ui->actionSync, SIGNAL(triggered()), this, SLOT(sync()));
    connect(ui->noteTitle, SIGNAL(textEdited(QString)), this, SLOT(noteTitleChange(QString)));
    connect(ui->actionAccount_info, SIGNAL(triggered()), this, SLOT(showUserInfo()));
    connect(ui->editor->page(), SIGNAL(linkClicked(QUrl)), this, SLOT(openURL(QUrl)));
    connect(ui->editor->page(), SIGNAL(microFocusChanged()), this, SLOT(updateEditButtonsState()));
    connect(ui->editor, SIGNAL(selectionChanged()), this, SLOT(updateSelectionButtonsState()));
    connect(ui->editor->page(), SIGNAL(downloadRequested(QNetworkRequest)), this, SLOT(downloadRequested(QNetworkRequest)));
    connect(ui->editor, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(editorContextMenuRequested(QPoint)));
    connect(ui->editor, SIGNAL(fileInserted(QString)), this, SLOT(insertFile(QString)));
    connect(ui->notebooks, SIGNAL(noteMoved(QString,QString)), this, SLOT(moveNote(QString,QString)));
    connect(ui->notebooks, SIGNAL(noteDeleted(QString)), this, SLOT(deleteNote(QString)));
    connect(ui->notebooks, SIGNAL(noteRestored(QString,QString)), this, SLOT(restoreNote(QString,QString)));
    connect(ui->NotesList, SIGNAL(noteDeleted(QString)), this, SLOT(deleteNote(QString)));
    connect(ui->NotesList, SIGNAL(noteRestored(QString)), this, SLOT(restoreNote(QString)));
    connect(ui->NotesList, SIGNAL(noteCreated()), this, SLOT(newNote()));
    connect(ui->NotesList, SIGNAL(reloaded()), this, SLOT(updateCurrentNoteName()));
    connect(ui->editButton, SIGNAL(toggled(bool)), this, SLOT(setEditable(bool)));
    connect(ui->actionDelete_Note, SIGNAL(triggered()), this, SLOT(deleteNote()));
    connect(ui->toolBox, SIGNAL(currentChanged(int)), this, SLOT(changeTab(int)));
    connect(ui->editor->page(), SIGNAL(linkHovered(QString,QString,QString)), this, SLOT(linkHovered(QString,QString,QString)));
    connect(jsB, SIGNAL(hintMessage(QString,int)), ui->statusbar, SLOT(showMessage(QString,int)));
    connect(jsB, SIGNAL(noteChanged(QString)), this, SLOT(updateTagsToolBar(QString)));
    connect(jsB, SIGNAL(activeNoteSelectionChanged(bool)), ui->actionDelete_Note, SLOT(setEnabled(bool)));
    connect(jsB, SIGNAL(activeNoteSelectionChanged(bool)), ui->editButton, SLOT(setEnabled(bool)));
    connect(jsB, SIGNAL(editingStarted(bool)), ui->editButton, SLOT(setChecked(bool)));
    connect(jsB, SIGNAL(titleUpdated(QString,QString)), this, SLOT(updateNoteTitle(QString,QString)));
    connect(jsB, SIGNAL(conflictAdded(qint64,QString,bool)), this, SLOT(addConflict(qint64,QString,bool)));
    connect(jsB, SIGNAL(noteSelectionChanged(bool)), ui->actionNote_Info, SLOT(setEnabled(bool)));
    connect(jsB, SIGNAL(noteSelectionChanged(bool)), ui->actionExport, SLOT(setEnabled(bool)));
    connect(jsB, SIGNAL(noteSelectionChanged(bool)), ui->actionPrint, SLOT(setEnabled(bool)));
    connect(ui->editor, SIGNAL(tagUpdated(QString,bool)), this, SLOT(updateTag(QString,bool)));
    connect(tagsActions, SIGNAL(triggered(QAction*)), this, SLOT(tagClicked(QAction*)));
    connect(newTag, SIGNAL(tagCreated(QString)), this, SLOT(createTag(QString)));
    connect(ui->actionKeep_only_this_Version, SIGNAL(triggered()), this, SLOT(keepThisVersion()));
    connect(ui->actionNote_Info, SIGNAL(triggered()), this, SLOT(showNoteInfo()));
    connect(ui->actionExport, SIGNAL(triggered()), this, SLOT(exportNote()));
    connect(ui->actionPrint, SIGNAL(triggered()), this, SLOT(print()));
    connect(this, SIGNAL(titleChanged(QString,QString)), jsB, SIGNAL(titleChanged(QString,QString)));


    setWindowIcon(appIcon);
    setWindowTitle("Hippo Notes");

    setTabOrder(ui->noteTitle, ui->editor);

    ui->notebooks->setSortingEnabled(true);
    ui->notebooks->sortByColumn(0, Qt::AscendingOrder);
    ui->editBar->setVisible(false);

    ui->mainToolBar->addAction(ui->actionNew_Note);
    ui->mainToolBar->addAction(ui->actionSync);

    QFontComboBox *font = new QFontComboBox(this);
    font->setDisabled(true);
    font->setFontFilters(QFontComboBox::ScalableFonts);
    font->setEditable(false);
    connect(this, SIGNAL(editButtonsStateChanged(bool)), font, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(updateFont(QFont)), font, SLOT(setCurrentFont(QFont)));
    connect(font, SIGNAL(activated(QString)), this, SLOT(changeFont(QString)));
    ui->editBar->addWidget(font);

    QComboBox *fontSize = new QComboBox(this);
    fontSize->setDisabled(true);
    fontSize->setEditable(false);
    for (int i = 1; i <= 7; i++)
        fontSize->addItem(QString::number(i));
    connect(this, SIGNAL(editButtonsStateChanged(bool)), fontSize, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(updateFontSize(int)), fontSize, SLOT(setCurrentIndex(int)));
    connect(fontSize, SIGNAL(activated(QString)), this, SLOT(changeFontSize(QString)));
    ui->editBar->addWidget(fontSize);

    QAction *boldIco = ui->editor->pageAction(QWebPage::ToggleBold);
    boldIco->setIcon(QIcon::fromTheme("format-text-bold"));
    ui->editBar->addAction(boldIco);

    QAction *italicIco = ui->editor->pageAction(QWebPage::ToggleItalic);
    italicIco->setIcon(QIcon::fromTheme("format-text-italic"));
    ui->editBar->addAction(italicIco);

    QAction *underlineIco = ui->editor->pageAction(QWebPage::ToggleUnderline);
    underlineIco->setIcon(QIcon::fromTheme("format-text-underline"));
    ui->editBar->addAction(underlineIco);

    QAction *strikethroughIco = ui->editor->pageAction(QWebPage::ToggleStrikethrough);
    strikethroughIco->setIcon(QIcon::fromTheme("format-text-strikethrough"));
    ui->editBar->addAction(strikethroughIco);

    ui->editBar->addSeparator();

    QAction *undoIco = ui->editor->pageAction(QWebPage::Undo);
    undoIco->setIcon(QIcon::fromTheme("edit-undo"));
    undoIco->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Z));
    ui->editBar->addAction(undoIco);
    ui->menu_Edit->addAction(undoIco);

    QAction *redoIco = ui->editor->pageAction(QWebPage::Redo);
    redoIco->setIcon(QIcon::fromTheme("edit-redo"));
    redoIco->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z));
    ui->editBar->addAction(redoIco);
    ui->menu_Edit->addAction(redoIco);

    ui->editBar->addSeparator();

    QAction *rformatIco = ui->editor->pageAction(QWebPage::RemoveFormat);
    rformatIco->setIcon(QIcon::fromTheme("edit-clear"));
    ui->editBar->addAction(rformatIco);

    ui->editBar->addSeparator();

    QAction *leftIco = ui->editor->pageAction(QWebPage::AlignLeft);
    leftIco->setIcon(QIcon::fromTheme("format-justify-left"));
    ui->editBar->addAction(leftIco);

    QAction *centerIco = ui->editor->pageAction(QWebPage::AlignCenter);
    centerIco->setIcon(QIcon::fromTheme("format-justify-center"));
    ui->editBar->addAction(centerIco);

    QAction *rightIco = ui->editor->pageAction(QWebPage::AlignRight);
    rightIco->setIcon(QIcon::fromTheme("format-justify-right"));
    ui->editBar->addAction(rightIco);

    QAction *fillIco = ui->editor->pageAction(QWebPage::AlignJustified);
    fillIco->setIcon(QIcon::fromTheme("format-justify-fill"));
    ui->editBar->addAction(fillIco);

    ui->editBar->addSeparator();

    QAction *indentIco = ui->editor->pageAction(QWebPage::Indent);
    indentIco->setIcon(QIcon::fromTheme("format-indent-more"));
    ui->editBar->addAction(indentIco);

    QAction *outdentIco = ui->editor->pageAction(QWebPage::Outdent);
    outdentIco->setIcon(QIcon::fromTheme("format-indent-less"));
    ui->editBar->addAction(outdentIco);

    QAction *superscriptIco = ui->editor->pageAction(QWebPage::ToggleSuperscript);
    superscriptIco->setIcon(QIcon::fromTheme("format-text-superscript"));
    ui->editBar->addAction(superscriptIco);

    QAction *subscriptIco = ui->editor->pageAction(QWebPage::ToggleSubscript);
    subscriptIco->setIcon(QIcon::fromTheme("format-text-subscript"));
    ui->editBar->addAction(subscriptIco);

    QAction *unorderedIco = ui->editor->pageAction(QWebPage::InsertUnorderedList);
    unorderedIco->setIcon(QIcon::fromTheme("format-list-unordered"));
    ui->editBar->addAction(unorderedIco);

    QAction *orderedIco = ui->editor->pageAction(QWebPage::InsertOrderedList);
    orderedIco->setIcon(QIcon::fromTheme("format-list-ordered"));
    ui->editBar->addAction(orderedIco);

    QAction *lineIco = new QAction("Insert horizontal line", this);
    lineIco->setIcon(QIcon::fromTheme("insert-horizontal-rule"));
    lineIco->setDisabled(true);
    connect(this, SIGNAL(editButtonsStateChanged(bool)), lineIco, SLOT(setEnabled(bool)));
    connect(lineIco, SIGNAL(triggered()), this, SLOT(insertHorizontalLine()));
    ui->editBar->addAction(lineIco);

    ui->menu_Edit->addSeparator();

    QAction *cutIco = ui->editor->pageAction(QWebPage::Cut);
    cutIco->setIcon(QIcon::fromTheme("edit-cut"));
    cutIco->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_X));
    ui->menu_Edit->addAction(cutIco);

    QAction *copyIco = ui->editor->pageAction(QWebPage::Copy);
    copyIco->setIcon(QIcon::fromTheme("edit-copy"));
    copyIco->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    ui->menu_Edit->addAction(copyIco);

    QAction *pasteIco = ui->editor->pageAction(QWebPage::Paste);
    pasteIco->setIcon(QIcon::fromTheme("edit-paste"));
    pasteIco->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_V));
    ui->menu_Edit->addAction(pasteIco);

    QAction *pasteSpecialIco = ui->editor->pageAction(QWebPage::PasteAndMatchStyle);
    pasteSpecialIco->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_V));
    ui->menu_Edit->addAction(pasteSpecialIco);

    ui->menu_Edit->addSeparator();

    QAction *insertUrlIco = new QAction("Create link", this);
    insertUrlIco->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_L));
    insertUrlIco->setDisabled(true);
    insertUrlIco->setIcon(QIcon::fromTheme("insert-link"));
    connect(this, SIGNAL(selectionButtonsStateChanged(bool)), insertUrlIco, SLOT(setDisabled(bool)));
    connect(insertUrlIco, SIGNAL(triggered()), this, SLOT(insertUrl()));
    ui->menu_Edit->addAction(insertUrlIco);

    QAction *todoIco = new QAction("Insert To-do Checkbox", this);
    todoIco->setIcon(QIcon::fromTheme("checkbox"));
    todoIco->setDisabled(true);
    connect(this, SIGNAL(editButtonsStateChanged(bool)), todoIco, SLOT(setEnabled(bool)));
    connect(todoIco, SIGNAL(triggered()), jsB, SIGNAL(insertToDo()));
    ui->menu_Edit->addAction(todoIco);

    QAction *insertImage = new QAction("Insert Image", this);
    insertImage->setIcon(QIcon::fromTheme("insert-image"));
    insertImage->setDisabled(true);
    connect(this, SIGNAL(editButtonsStateChanged(bool)), insertImage, SLOT(setEnabled(bool)));
    connect(insertImage, SIGNAL(triggered()), this, SLOT(insertImg()));
    ui->menu_Edit->addAction(insertImage);

    QAction *insertFile = new QAction("Insert File", this);
    insertFile->setDisabled(true);
    connect(this, SIGNAL(editButtonsStateChanged(bool)), insertFile, SLOT(setEnabled(bool)));
    connect(insertFile, SIGNAL(triggered()), this, SLOT(insertFile()));
    ui->menu_Edit->addAction(insertFile);

    QAction *encryptIco = new QAction("Encrypt Selected Text...", this);
    encryptIco->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_X));
    encryptIco->setDisabled(true);
    encryptIco->setIcon(QIcon::fromTheme("document-edit-encrypt"));
    connect(this, SIGNAL(selectionButtonsStateChanged(bool)), encryptIco, SLOT(setDisabled(bool)));
    connect(encryptIco, SIGNAL(triggered()), jsB, SIGNAL(encryptText()));
    ui->menu_Edit->addAction(encryptIco);

    ui->menu_Edit->addSeparator();

    QAction *options = new QAction("&Options...", this);
    options->setIcon(QIcon::fromTheme("preferences-other"));
    connect(options, SIGNAL(triggered()), this, SLOT(showOptions()));
    ui->menu_Edit->addAction(options);


    clearConflictBar();
    connect(jsB, SIGNAL(showConflict()), ui->conflictBar, SLOT(show()));
    connect(jsB, SIGNAL(showConflict()), ui->conflictBarBottom, SLOT(show()));

    conflictsGroup = new QActionGroup(this);
    connect(conflictsGroup, SIGNAL(triggered(QAction*)), this, SLOT(changeNoteVersion(QAction*)));

    searchIndex = new SearchIndex(this);
    connect(ui->buildIndex, SIGNAL(clicked()), searchIndex, SLOT(buildSearchIndex()));
    connect(ui->searchButton, SIGNAL(clicked()), this, SLOT(search()));

    //showWindow();
    edam->init();
}

void MainWindow::closeWindow()
{
    ui->actionClose->setDisabled(true);
    disconnect(EdamProtocol::GetInstance(), SIGNAL(syncFinished()), this, SLOT(syncFinished()));

    sync();

    qApp->quit();
}

MainWindow::~MainWindow()
{
    delete ui;
    EdamProtocol::deleteInstance();
    Speller::deleteInstance();
}

void MainWindow::authentificationFailed()
{
    QMessageBox msgBox;
    msgBox.setText("Authentification failed!");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();

    QTimer::singleShot(250, qApp, SLOT(quit()));
}

void MainWindow::syncStarted(int count)
{
    qDebug() << "syncStarted()" << count;

    ui->actionSync->setEnabled(false);
    if (loaded)
        return;    

    QProgressDialog* progress = new QProgressDialog("Syncing Notes...", "Abort Sync", 0, count, this);
    progress->setAutoReset(false);
    progress->setAutoClose(false);
    connect( EdamProtocol::GetInstance(), SIGNAL(syncProgress(int)), progress, SLOT(setValue(int)));
    connect(progress, SIGNAL(canceled()), EdamProtocol::GetInstance(), SLOT(cancelSync()));
    connect(progress, SIGNAL(canceled()), progress, SLOT(deleteLater()));
    connect(EdamProtocol::GetInstance(), SIGNAL(syncFinished()), progress, SLOT(close()));
    connect(EdamProtocol::GetInstance(), SIGNAL(syncRangeChange(int)), progress, SLOT(setMaximum(int)));
    progress->show();

    ui->statusbar->showMessage("Sync Started!", 5000);
}

void MainWindow::syncFinished()
{
    qDebug() << "syncFinished()";

    ui->actionSync->setEnabled(true);
    ui->notebooks->reload();
    ui->tags->reload();
    updateTagsToolBar();
    loadSelectionState(loaded);

    if (!loaded) {
        loaded = true;
        showWindow();
        return;
    }

    ui->statusbar->showMessage("Sync Finished!", 5000);
}

void MainWindow::showWindow()
{
    loadAboutInfo();

    show();
    qApp->setQuitOnLastWindowClosed(true);    

}

void MainWindow::switchNotebook()
{
    TreeItem* n = reinterpret_cast<TreeItem*>(ui->notebooks->currentItem());

    if (n == NULL)
        return;

    ui->NotesList->switchNotebook(n->getType(), n->getGUIDs(), getCurrentNoteGuid());
}

void MainWindow::switchTag()
{
    qDebug() << "switchTag()";

    if (ui->toolBox->currentIndex() != 1)
        return;

    QStringList tagGuids = ui->tags->currentGuids();
    if (tagGuids.isEmpty())
        return;

    ui->NotesList->switchTag(tagGuids, getCurrentNoteGuid());
}

void MainWindow::switchNote()
{
    qDebug() << "switchNote()";
    ListItem* l = reinterpret_cast<ListItem*>(ui->NotesList->currentItem());
    QString id = l->getGUID();

    if (id.isEmpty())
        return;

    ui->noteTitle->clear();
    ui->titleBar->show();
    ui->editButton->setChecked(false);
    clearConflictBar();

    JS(QString("loadNote('%1', false);").arg(id));
}

void MainWindow::loadAboutInfo()
{
    ui->NotesList->clearSelection();
    ui->noteTitle->clear();
    ui->titleBar->hide();
    clearConflictBar();
    ui->actionDelete_Note->setDisabled(true);
    JS("loadAboutInfo();");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (trayIcon && trayIcon->isVisible())
        this->hide();
    else
        closeWindow();

     event->ignore();
}

void MainWindow::trayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
    if (reason != QSystemTrayIcon::Trigger)
        return;

    if (!loaded)
        return;

    if (this->isHidden())
        this->show();
    else
        this->hide();
}

void MainWindow::addJSObject()
{
    ui->editor->page()->mainFrame()->addToJavaScriptWindowObject(QString("jsB"), jsB);
    ui->editor->page()->mainFrame()->addToJavaScriptWindowObject(QString("enmlWritter"), enmlWritter);
}

void MainWindow::newNote()
{
    TreeItem* n = reinterpret_cast<TreeItem*>(ui->notebooks->currentItem());
    if (n->getType() == TreeItem::stack) {
        if (n->childCount() > 0) {
            n = reinterpret_cast<TreeItem*>(n->child(0));
            ui->notebooks->setCurrentItem(n);
        } else
            n = NULL;
    }
    if ((n->getType() == TreeItem::allNotes) || (n->getType() == TreeItem::trashBin) || (n == NULL)) {
        if (ui->notebooks->defaultNotebook() == NULL)
            return;

        ui->notebooks->setCurrentItem(ui->notebooks->defaultNotebook());
        n = ui->notebooks->defaultNotebook();
    }

    QString nb = n->getGUIDs().first();

    Note *note = new Note();
    QString guid = note->createNewNote(nb);
    delete note;

    ui->editButton->setChecked(false);
    JS(QString("loadNote('%1', true);").arg(guid));
    ui->titleBar->show();
    ui->toolBox->setCurrentIndex(0);
    ui->NotesList->switchNotebook(TreeItem::noteBook, n->getGUIDs(), guid);
    ui->notebooks->updateCounts();

}

void MainWindow::noteTitleChange(QString newTitle)
{
    QString guid = ui->noteTitle->objectName();

    if (guid.isEmpty())
        return;

    emit titleChanged(newTitle, guid);

    ListItem *note = ui->NotesList->getNoteWithGuid(guid);
    if (note == NULL)
        return;

    note->setText(newTitle);
}

void MainWindow::sync() {
    JS("checkModified();");
    saveSelectionState();
    EdamProtocol::GetInstance()->sync();
}

void MainWindow::moveNote(QString note, QString notebook) {

    QSqlQuery query;
    query.prepare("UPDATE notes SET notebookGuid=:notebookGuid WHERE guid=:guid");
    query.bindValue(":notebookGuid", notebook);
    query.bindValue(":guid", note);
    query.exec();

    Note::editField(note, Note::T_NOTEBOOK_GUID);

    saveSelectionState("notebook@" + notebook);
    ui->notebooks->reload();
    loadSelectionState(true);
}

void MainWindow::openURL(QUrl url) {
    QDesktopServices::openUrl(url);
}

void MainWindow::setEditable(bool enabled) {

    qDebug() << "setEditable() "<< enabled;

    QString guid = getCurrentNoteGuid();
    if (enabled && guid.isEmpty())
        return;

    ui->editBar->setVisible(enabled);
    ui->noteTitle->setReadOnly(!enabled);

    JS("setEditable(" + QVariant(enabled).toString() + ");");

    if (enabled) {
        ui->editor->setFocus();
        ui->tagsBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    } else
        ui->tagsBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
}

void MainWindow::updateEditButtonsState() {
    editingEnabled = JS("isOnEditableArea();").toBool();
    emit editButtonsStateChanged(editingEnabled);

    if (editingEnabled) {
         QString f = JS("document.queryCommandValue('fontName');").toString();
         emit updateFont(QFont(f));

         int size = JS("document.queryCommandValue('fontSize');").toInt();
         emit updateFontSize(size - 1);
    }
}

void MainWindow::insertHorizontalLine() {
    JS("document.execCommand('insertHorizontalRule',false,null);");
}

void MainWindow::updateSelectionButtonsState() {
    emit selectionButtonsStateChanged(!editingEnabled || ui->editor->selectedText().isEmpty());
}

void MainWindow::insertUrl() {
    URLDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString cmd = QString("document.execCommand('CreateLink',false,'%1');").arg(dialog.getURL());
        JS(cmd);
    }
}

void MainWindow::deleteNote(QString note) {
    qDebug() << "deleteNote()";

    if (note.isEmpty())
        note = getCurrentNoteGuid();

    if (note.isEmpty())
        return;

    Note *n = Note::fromGUID(note);

    if (n == NULL)
        return;

    QMessageBox msgBox(this);
    msgBox.setText("Are you sure you want to delete this Note?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle(n->getTitle());
    msgBox.exec();

    if(msgBox.result() != QMessageBox::Yes)
        return;

    qDebug() << note;

    saveSelectionState();

    Note::NoteUpdates updates;
    updates[Note::T_ACTIVE] = false;
    updates[Note::T_DELETED] = QDateTime::currentDateTime().toMSecsSinceEpoch();
    n->update(updates);

    delete n;
    ui->notebooks->reload();
    ui->tags->reload();
    loadSelectionState();
}

void MainWindow::saveSelectionState(QString notebook) {
    qDebug() << "saveSelectionState()";

    QSqlQuery query;
    query.exec("DELETE FROM syncStatus WHERE option IN ('selNotebook', 'selTab', 'selNote', 'selTag')");

    sql::updateSyncStatus("selTab", ui->toolBox->currentIndex());

    if (notebook.isEmpty() && (ui->notebooks->currentItem() != NULL)) {
        TreeItem* n = reinterpret_cast<TreeItem*>(ui->notebooks->currentItem());
        notebook = n->getName();
    }
    if (!notebook.isEmpty())
        sql::updateSyncStatus("selNotebook", notebook);

    QStringList tagGuids = ui->tags->currentGuids();
    if (!tagGuids.isEmpty())
        sql::updateSyncStatus("selTag", tagGuids.first());

    if (ui->NotesList->currentItem() == NULL)
        return;

    ListItem* l = reinterpret_cast<ListItem*>(ui->NotesList->currentItem());
    sql::updateSyncStatus("selNote", l->getGUID());

    qDebug() << l->getGUID();
}

void MainWindow::loadSelectionState(bool selectNote) {
    qDebug() << "loadSelectionState()";

    QSqlQuery query;
    query.exec("SELECT option, value FROM syncStatus WHERE option IN ('selNotebook', 'selTab', 'selNote', 'selTag')");

    QHash<QString, QString> status;    

    while (query.next())
        status[query.value(0).toString()] = query.value(1).toString();

    if (!status.contains("selTab"))
        return;

    qDebug() << status;

    int tab = status["selTab"].toInt();

    ui->toolBox->setCurrentIndex(tab);

    ui->NotesList->blockSignals(true);

    QList<QString> array;
    array.append("selNotebook");
    array.append("selTag");
    array.append("selSearch");
    array.move(tab, array.size()-1);

    for (int i = 0; i< array.size(); i++) {
        QString name = array.at(i);
        if (!status.contains(name))
            continue;

        if (name == "selNotebook")
            ui->notebooks->selectNotebookWithName(status["selNotebook"]);

        if (name == "selTag")
            ui->tags->selectTagWithGUID(status["selTag"]);
    }

    if (selectNote && status.contains("selNote")) {
        ListItem* note = ui->NotesList->getNoteWithGuid(status["selNote"]);
        if (note != NULL )
            ui->NotesList->setCurrentItem(note);
    }

    ui->NotesList->blockSignals(false);
}

void MainWindow::changeFont(QString font) {
    JS("document.execCommand('fontName',false,'"+font+"');");
}

void MainWindow::changeFontSize(QString fontSize) {
    JS("document.execCommand('fontSize',false,'"+fontSize+"');");
}

void MainWindow::insertImg() {
    QString note = getCurrentNoteGuid();
    insertImage dialog(this, note);

    if (dialog.exec() != QDialog::Accepted)
        return;

    if (dialog.getType() == insertImage::IMG_NONE)
        return;

    if (dialog.getType() == insertImage::IMG_URL)
        JS("document.execCommand('insertImage',false,'"+dialog.getUrl()+"');");
    else if (dialog.getType() == insertImage::IMG_LOCAL)
        JS("insertImage('"+ dialog.getBodyHash() + "', '" + dialog.mimeType() + "');");
}

QString MainWindow::getCurrentNoteGuid() {
    QString guid = JS("guid").toString();
    if (guid != "main")
        return guid;
    return "";
}

void MainWindow::changeTab(int index) {
    if (!loaded)
        return;

    if (index == 0)
        switchNotebook();
    else if (index == 1)
        switchTag();
}

void MainWindow::linkHovered(QString link, QString title, QString textContent) {
    Q_UNUSED(title);
    Q_UNUSED(textContent);

    QUrl url = QUrl(link);
    if (!url.isValid())
        return;

    QStringList validSchemes;
    validSchemes << "http";
    validSchemes << "https";
    validSchemes << "ftp";
    validSchemes << "sftp";
    validSchemes << "irc";
    validSchemes << "ssh";
    validSchemes << "git";

    if (!validSchemes.contains(url.scheme(), Qt::CaseInsensitive))
        return;

    ui->statusbar->showMessage(link);
}

void MainWindow::showUserInfo() {
    UserInfo userInfoWindow(this);
    userInfoWindow.exec();
}

void MainWindow::downloadRequested( const QNetworkRequest & request) {
    qDebug() << "downloadRequested()" << request.url();

    if (request.url().scheme() == "resource") {
        QString guid = request.url().host();
        jsB->saveAsResource(guid);
    } else if ((request.url().scheme() == "http") || (request.url().scheme() == "https")) {
        EdamProtocol::GetInstance()->getNetworkManager()->downloadReply(request);
    }
}

void MainWindow::restoreNote(QString note, QString notebook) {

    Note *n = Note::fromGUID(note);

    if (n == NULL)
        return;

    Note::NoteUpdates updates;
    updates[Note::T_ACTIVE] = true;

    if (!notebook.isEmpty())
        updates[Note::T_NOTEBOOK_GUID] = notebook;

    n->update(updates);
    saveSelectionState("notebook@" + n->getNotebookGuid());

    delete n;

    ui->notebooks->reload();
    loadSelectionState(true);
}

void MainWindow::updateTag(QString guid, bool checked) {    
    updateTag(getCurrentNoteGuid(), guid, checked);
}

void MainWindow::updateTag(QString noteGuid, QString guid, bool checked) {
    qDebug() << "updateTag" << noteGuid << guid << checked;

    if (noteGuid.isEmpty())
        return;

    QSqlQuery query;

    if (checked)
        query.prepare("REPLACE INTO notesTags (noteGuid, guid) VALUES (:noteGuid, :guid)");
    else
        query.prepare("DELETE FROM notesTags WHERE noteGuid=:noteGuid AND guid=:guid");

    query.bindValue(":noteGuid", noteGuid);
    query.bindValue(":guid", guid);
    query.exec();

    qDebug() << query.lastError().text();

    Note::editField(noteGuid, Note::T_TAG_GUIDS);

    ui->tags->updateCounts();
    switchTag();
    if (getCurrentNoteGuid() == noteGuid)
        updateTagsToolBar(noteGuid);
}

void MainWindow::changeNoteGuid(QString oldGuid, QString newGuid) {
    if (getCurrentNoteGuid() == oldGuid)
        JS(QString("guid='%1';").arg(newGuid));
}

void MainWindow::editorContextMenuRequested ( const QPoint & pos ) {
    QString guid = getCurrentNoteGuid();
    if (guid == "main")
        return;

    qDebug() << "editorContextMenuRequested" << pos;

    QWebHitTestResult element = ui->editor->page()->mainFrame()->hitTestContent(pos);

    if (element.isNull())
        return;

    QStringList classes = allClasses(element.element());
    if (classes.contains("pdfarea"))
        return;

    qDebug() << classes;

    QMenu * menu = new QMenu(ui->editor);

    menu->addAction(ui->editor->pageAction(QWebPage::SelectAll));

    if (element.isContentSelected()) {

        if (!menu->isEmpty())
            menu->addSeparator();

        menu->addAction(ui->editor->pageAction(QWebPage::Copy));

        if (element.isContentEditable()) {
            menu->addAction(ui->editor->pageAction(QWebPage::Cut));
            menu->addAction(ui->editor->pageAction(QWebPage::Paste));
            menu->addSeparator();

            menu->addAction(ui->editor->pageAction(QWebPage::ToggleBold));
            menu->addAction(ui->editor->pageAction(QWebPage::ToggleItalic));
            menu->addAction(ui->editor->pageAction(QWebPage::ToggleUnderline));
        }
    }

    if(!element.imageUrl().isEmpty() && (element.imageUrl().scheme() != "qrc")) {

        if (!menu->isEmpty())
            menu->addSeparator();

        menu->addAction(ui->editor->pageAction(QWebPage::DownloadImageToDisk));
        menu->addAction(ui->editor->pageAction(QWebPage::CopyImageToClipboard));

        if (element.imageUrl().scheme() == "http" || element.imageUrl().scheme() == "https")
            menu->addAction(ui->editor->pageAction(QWebPage::CopyImageUrlToClipboard));

        QAction * openImage = new QAction(this);
        openImage->setText("Open Image");
        openImage->setObjectName("openImage");
        menu->addAction(openImage);

        if (JS("editMode").toBool()) {
            menu->addSeparator();

            QAction * deleteImage = new QAction(this);
            deleteImage->setText("Delete Image");
            deleteImage->setObjectName("deleteImage");
            deleteImage->setIcon(QIcon::fromTheme("edit-delete"));
            menu->addAction(deleteImage);

            if (element.imageUrl().scheme() != "resource") {
                QAction * saveLocally = new QAction(this);
                saveLocally->setText("Save Locally");
                saveLocally->setObjectName("saveLocally");
                menu->addAction(saveLocally);
            }
        }
    }

    if (!element.linkUrl().isEmpty()) {
        if (!menu->isEmpty())
            menu->addSeparator();

        menu->addAction(ui->editor->pageAction(QWebPage::CopyLinkToClipboard));

        if (element.isContentEditable()) {
            QAction * deleteURL = new QAction(this);
            deleteURL->setText("Remove Link");
            deleteURL->setObjectName("deleteURL");
            menu->addAction(deleteURL);
        }
    }

    if (element.isContentEditable() && !element.isContentSelected() && element.imageUrl().isEmpty()) {
        Speller *speller = Speller::GetInstance();
        if (speller->initialized()) {

            QHash<QString, QString> languages = speller->availableLanguages();
            if (!languages.isEmpty()) {

                if (!menu->isEmpty())
                    menu->addSeparator();                

                QAction* act = menu->addAction(tr("Check &Spelling"));
                act->setCheckable(true);
                act->setChecked(speller->isEnabled());
                connect(act, SIGNAL(triggered(bool)), speller, SLOT(setEnabled(bool)));

                if (speller->isEnabled()) {
                    QString word = JS(QString("getSpellingWord(%1,%2);").arg(pos.x()).arg(pos.y())).toString();
                    if (!word.isEmpty() && speller->isMisspelled(word)) {
                        QStringList wordsList = speller->suggest(word);

                        QFont boldFont = menu->font();
                        boldFont.setBold(true);

                        QActionGroup *suggestGroup = new QActionGroup(menu);

                        QString suggest;
                        foreach(suggest, wordsList) {
                            QAction* act = menu->addAction(suggest);
                            act->setFont(boldFont);
                            act->setData(suggest);
                            suggestGroup->addAction(act);
                        }
                        connect(suggestGroup, SIGNAL(triggered(QAction*)), this, SLOT(replaceWord(QAction*)));
                    }
                }

                QMenu* mLanguages = new QMenu("Languages", menu);
                menu->addMenu(mLanguages);
                QActionGroup *mLanguagesG = new QActionGroup(mLanguages);
                mLanguagesG->setExclusive(true);

                QString lCode;
                foreach (lCode, languages.keys()) {
                    QAction* act = mLanguages->addAction(languages[lCode]);
                    act->setCheckable(true);
                    act->setChecked(lCode == speller->language());
                    act->setObjectName("language");
                    act->setData(lCode);
                    mLanguagesG->addAction(act);
                }
            }
        }
    }

    if (classes.contains("cryptButton")) {
        QAction * decrypt = new QAction(this);
        decrypt->setText("Decrypt...");
        decrypt->setObjectName("decrypt");
        decrypt->setIcon(QIcon::fromTheme("document-edit-decrypt"));
        menu->addAction(decrypt);
    }
    if (classes.contains("encrypted")) {
        QAction * closeCrypt = new QAction(this);
        closeCrypt->setText("Hide encrypted content");
        closeCrypt->setObjectName("closeCrypt");
        closeCrypt->setIcon(QIcon::fromTheme("dialog-close"));
        menu->addAction(closeCrypt);
    }

    QAction *ret = menu->exec(ui->editor->mapToGlobal(pos));

    if (ret == NULL)
        return;

    if (ret->objectName() == "openImage") {
        if (element.imageUrl().scheme() == "http" || element.imageUrl().scheme() == "https")
            openURL(element.imageUrl());
        else if (element.imageUrl().scheme() == "resource")
            jsB->externalOpenResource(element.imageUrl().host());
    } else if (ret->objectName() == "deleteImage") {
       if (element.imageUrl().scheme() == "resource") {
           QWebElement item = element.element();
           while (!item.isNull() && (item.tagName().toLower() != "en-media"))
               item = item.parent();

           if (item.tagName().toLower() == "en-media")
               item.removeFromDocument();
       } else
           element.element().removeFromDocument();

       JS("contentModified=true;");
    } else if (ret->objectName() == "deleteURL") {
        JS("document.execCommand('unlink',false,null);");
    } else if (ret->objectName() == "language") {
        Speller::GetInstance()->setLanguage(ret->data().toString());
    } else if (ret->objectName() == "decrypt") {
        element.element().evaluateJavaScript("this.click();");
    } else if (ret->objectName() == "closeCrypt") {
        element.element().evaluateJavaScript("hideCrypt(this);");
    } else if (ret->objectName() == "saveLocally") {
        element.element().evaluateJavaScript("saveLocally(this);");
    }
}

void MainWindow::insertFile() {
    qDebug() << "insertFile()";

    QStringList filetypes;

    filetypes.append("PDF Documents (*.pdf)");
    filetypes.append("Images (*.jpg *.jpeg *.png *.gif)");

    QString fileName = QFileDialog::getOpenFileName(this, "Insert File", QDir::homePath(), filetypes.join(";;"));
    insertFile(fileName);
}

void MainWindow::insertFile(QString fileName) {
    if (fileName.isEmpty())
        return;

    Resource res;
    res.setNoteGuid(getCurrentNoteGuid());
    if (res.create(fileName)) {
        if (res.isImage())
            JS("insertImage('"+ res.getBodyHash() + "', '" + res.mimeType() + "');");
        else if (res.isPDF())
            JS("insertPDF('"+ res.getBodyHash() + "');");
    }
}

QVariant MainWindow::JS(QString command) {
    return ui->editor->page()->mainFrame()->evaluateJavaScript(command);
}

void MainWindow::updateNoteTitle(QString guid, QString title) {
    ui->noteTitle->setObjectName(guid);
    ui->noteTitle->setText(title);
}

void MainWindow::replaceWord( QAction * action ) {
    JS(QString("replaceSpellWord('%1');").arg(action->data().toString()));
}

QStringList MainWindow::allClasses(QWebElement element) {
    QStringList result;
    while (!element.isNull()) {
        result.append(element.classes());
        element = element.parent();
    }
    return result;
}

void MainWindow::updateTagsToolBar(QString noteGuid) {

    if (noteGuid.isEmpty())
        noteGuid = getCurrentNoteGuid();

    ui->tagsBar->clear();

    if (noteGuid.isEmpty())
        return;

    QSqlQuery result;
    result.prepare("SELECT notesTags.guid, tags.name FROM notesTags LEFT JOIN tags ON notesTags.guid = tags.guid WHERE notesTags.noteGuid=:noteGuid ORDER BY tags.name COLLATE NOCASE ASC");
    result.bindValue(":noteGuid", noteGuid);
    result.exec();

    while (result.next()) {
        QAction *action = ui->tagsBar->addAction(QIcon::fromTheme("edit-delete"), result.value(1).toString());
        action->setData(result.value(0));
        action->setActionGroup(tagsActions);
    }
    QAction *action = ui->tagsBar->addWidget(newTag);
    action->setVisible(ui->editButton->isChecked());

    connect(ui->editButton, SIGNAL(toggled(bool)), action, SLOT(setVisible(bool)));
}

void MainWindow::tagClicked(QAction * action) {
    QString guid = action->data().toString();
    if (guid.isEmpty())
        return;

    if (ui->editButton->isChecked())
        updateTag(guid, false);
    else {
        ui->toolBox->setCurrentIndex(1);
        ui->tags->selectTagWithGUID(guid);
    }
}

void MainWindow::createTag(QString tag) {
    qDebug() << "createTag" << tag;

    QSqlQuery query;
    query.prepare("SELECT guid FROM tags WHERE name=:name");
    query.bindValue(":name", tag);
    query.exec();

    QString tagGuid;

    if (query.next()) {
        tagGuid = query.value(0).toString();
    } else {
        Tag* t = new Tag();
        tagGuid = t->createNewTag(tag);
        delete t;

        ui->tags->reload();
    }

    updateTag(tagGuid, true);
}

void MainWindow::addTag(QString noteGuid, QString tagGuid) {
    updateTag(noteGuid, tagGuid, true);
}

void MainWindow::updateCurrentNoteName() {

    ListItem *item = ui->NotesList->getNoteWithGuid(getCurrentNoteGuid());
    if (item == NULL)
        return;

    QString name = JS("title").toString();
    if (name.isEmpty())
        return;

    item->setText(name);
}

void MainWindow::clearConflictBar() {
    ui->conflictBar->clear();
    ui->conflictBar->addWidget(new QLabel("<b>Conflicting note versions: </b>", this));
    ui->conflictBar->hide();

    ui->conflictBarBottom->hide();
}

void MainWindow::addConflict(qint64 date, QString hash, bool enabled) {
    QString name = QDateTime::fromMSecsSinceEpoch(date).toString();

    QAction *conflict = ui->conflictBar->addAction(name);
    conflict->setCheckable(true);
    conflict->setData(hash);

    conflictsGroup->addAction(conflict);

    conflict->setChecked(enabled);
}

void MainWindow::changeNoteVersion(QAction * action) {
    qDebug() << "changeNoteVersion()" << action->data().toString();

    if (action == NULL || action->data().toString().isEmpty())
        return;

    JS("loadVersion('"+action->data().toString()+"');");
}

void MainWindow::keepThisVersion() {
    qDebug() << "keepThisVersion()";

    QAction* action = conflictsGroup->checkedAction();
    if (action == NULL)
        return;

    QString hash = action->data().toString();
    QString guid = getCurrentNoteGuid();
    if (hash.isEmpty() || guid.isEmpty())
        return;

    Note *n = Note::fromGUID(guid);

    QVariantMap conflict = n->conflict();
    if (!conflict.isEmpty()) {
        if (n->getContentHash() != hash) {
            Note::NoteUpdates updates;
            updates[Note::T_CONTENT] = conflict["content"];
            updates[Note::T_UPDATED] = conflict["updated"];
            n->update(updates);
        }
        n->dropConflict();
    }
    delete n;

    ui->notebooks->reload();
    clearConflictBar();
    JS(QString("loadNote('%1', false);").arg(guid));
}

void MainWindow::showNoteInfo() {
    NoteInfoDialog dialog(this, getCurrentNoteGuid());
    dialog.exec();
}

void MainWindow::exportNote() {
    QStringList filters;
    filters << "PDF - Portable Document Format (*.pdf)";
    filters << "HTML (*.html)";
    filters << "Plain Text (*.txt)";

    QFileDialog dialog(this);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilters(filters);
    dialog.setWindowTitle("Save File As");
    dialog.setNameFilterDetailsVisible(true);
#if QT_VERSION >= 0x050000
    dialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).at(0) + QDir::separator());
#else
    dialog.setDirectory(QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator());
#endif
    dialog.selectFile(JS("title").toString());
    dialog.setDefaultSuffix("pdf");

    if (dialog.exec() != QDialog::Accepted)
        return;

    QString fileName = dialog.selectedFiles()[0];
    if (fileName.isEmpty())
        return;

    QString dir = dialog.directory().absolutePath();

    QString filter = dialog.selectedNameFilter();
    int idx = filters.indexOf(filter);

    if (idx == 0) {
        QPrinter printer;
        //printer.setPageMargins(20,5,5,5,QPrinter::Millimeter);
        printer.setOutputFileName(fileName);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setColorMode(QPrinter::Color);
        printer.setPageSize(QPrinter::A4);
        ui->editor->print(&printer);
    } else if (idx == 1 || idx == 2) {

        QString content;
        if (idx == 1)
            content = exportToHtml(dir);
        else
            content = JS("exportToText()").toString();

        QFile file( fileName );
        if ( file.open(QIODevice::WriteOnly) )
        {
            file.write(content.toUtf8());
        }
        file.close();
    }
}

QString MainWindow::exportToHtml(QString dir) {
    QVariantMap json = JS("exportToHtml()").toMap();
    QString content = json["content"].toString();

    QVariantList images = json["images"].toList();

    QVariant i;
    foreach(i, images) {
        QVariantMap img = i.toMap();
        QString id = img["id"].toString();
        QString imageFile = dir + QDir::separator() + img["fileName"].toString();

        Resource *res = Resource::fromHash(id);
        if (res == NULL)
            continue;

        QFile file( imageFile );
        if ( file.open(QIODevice::WriteOnly) )
        {
            file.write(res->getData());
        }
        file.close();

        delete res;
    }
    return content;
}

void MainWindow::print() {

    QPrinter printer;

    QPrintDialog *dialog = new QPrintDialog(&printer, this);
    dialog->setWindowTitle("Print Note");

    if (dialog->exec() != QDialog::Accepted)
        return;

    ui->editor->print(&printer);
}

void MainWindow::showOptions() {
    OptionsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QVariantMap settings = dialog.getSettings();

        enableSystemTrayIcon(settings["systemTray"].toBool());
        EdamProtocol::GetInstance()->setSyncInterval(settings["SyncInterval"].toInt());
        NetworkProxyFactory::GetInstance()->loadSettings();
    }
}

void MainWindow::enableSystemTrayIcon(bool state) {
    bool currState = (trayIcon != NULL) && trayIcon->isVisible();

    if (currState == state)
        return;

    if (state) {
        QMenu *trayMenu = new QMenu(this);
        trayMenu->addAction(ui->actionClose);

        trayIcon = new QSystemTrayIcon(appIcon, this);
        trayIcon->setToolTip("Hippo Notes - A Note Taking tool");
        trayIcon->setContextMenu(trayMenu);
        connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconClicked(QSystemTrayIcon::ActivationReason)));
        trayIcon->show();
    } else {
        disconnect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconClicked(QSystemTrayIcon::ActivationReason)));
        trayIcon->hide();
        trayIcon->deleteLater();
        trayIcon = NULL;
    }

}

void MainWindow::search() {

    QString query = ui->searchInput->text();

    if (query.isEmpty())
        return;

    QStringList guids = searchIndex->search(query);

    ui->NotesList->switchSearch(guids, getCurrentNoteGuid());
}
