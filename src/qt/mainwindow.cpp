/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://www.qtsoftware.com/contact.
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

#include "mainwindow.h"
#include "fractalmodel.h"
#include "fractalwidget.h"
#include "fractalparameters.h"

MainWindow::MainWindow()
{
    init();
    setCurrentFile("");
}

MainWindow::MainWindow(const QString &fileName)
{
    init();
    loadFile(fileName);
}

void MainWindow::closeEvent(QCloseEvent *event)
{

    if (maybeSave()) {
        writeSettings();
        event->accept();
    } else {
        event->ignore();
    }

}

void MainWindow::newFile()
{
    MainWindow *other = new MainWindow;
    other->move(x() + 40, y() + 40);
    other->show();
}

void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty()) {
        MainWindow *existing = findMainWindow(fileName);
        if (existing) {
            existing->show();
            existing->raise();
            existing->activateWindow();
            return;
        }

        if (isUntitled //&& textEdit->document()->isEmpty()
                && !isWindowModified()) {
            loadFile(fileName);
        } else {
            MainWindow *other = new MainWindow(fileName);
            if (other->isUntitled) {
                delete other;
                return;
            }
            other->move(x() + 40, y() + 40);
            other->show();
        }

    }
}

bool MainWindow::save()
{
    if (isUntitled) {
        return saveAs();
    } else {
        return saveFile(curFile);
    }
}

bool MainWindow::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                                                    curFile);
    if (fileName.isEmpty())
        return false;

    return saveFile(fileName);
}

void MainWindow::about()
{
   QMessageBox::about(this, tr("About XaoS"),
            tr("<p>"
               "XaoS - a realtime fractal zoomer<br>"
               "Copyright 1996-2009 Jan Hubicka, Thomas Marsh and others"
               "</p><hr><p>"
               "This program is free software; you can redistribute it and/or modify it under the "
               "terms of the GNU General Public License as published by the Free Software "
               "Foundation; either version 2 of the License, or (at your option) any later "
               "version."
               "</p><p>"
               "This program is distributed in the hope that it will be useful, but WITHOUT ANY "
               "WARRANTY; without even the implied warranty of MERCHANTABILITY or "
               "FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License "
               "for more details."
               "</p><p>"
               "You should have received a copy of the GNU General Public License along with "
               "this program; if not, write to the Free Software Foundation, Inc. 675 Mass Ave, "
               "Cambridge, MA 02139, USA."
               "</p>"));
}

void MainWindow::documentWasModified()
{
    setWindowModified(true);
}

void MainWindow::init()
{
    setAttribute(Qt::WA_DeleteOnClose);

    isUntitled = true;

    model = new FractalModel;
    connect(model, SIGNAL(messageChanged(QString)), statusBar(), SLOT(showMessage(QString)));
    connect(model, SIGNAL(messageCleared()), this, SLOT(clearMessage()));
    connect(model, SIGNAL(errorEncountered(QString)), this, SLOT(showError(QString)));


    parameters = new FractalParameters(this);
    parameters->setModel(model);

    QDockWidget *dw = new QDockWidget("Parameters", this);
    dw->setWidget(parameters);
    addDockWidget(Qt::RightDockWidgetArea, dw);

    view = new FractalWidget;
    view->setModel(model);
    setCentralWidget(view);

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    readSettings();

    //connect(textEdit->document(), SIGNAL(contentsChanged()),
    //        this, SLOT(documentWasModified()));

    setUnifiedTitleAndToolBarOnMac(true);
}

void MainWindow::createActions()
{
    newAct = new QAction(/*QIcon(":/images/new.png"),*/ tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));

    openAct = new QAction(/*QIcon(":/images/open.png"),*/ tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    saveAct = new QAction(/*QIcon(":/images/save.png"),*/ tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

    closeAct = new QAction(tr("&Close"), this);
    closeAct->setShortcut(tr("Ctrl+W"));
    closeAct->setStatusTip(tr("Close this window"));
    connect(closeAct, SIGNAL(triggered()), this, SLOT(close()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

    cutAct = new QAction(/*QIcon(":/images/cut.png"),*/ tr("Cu&t"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                            "clipboard"));
    //connect(cutAct, SIGNAL(triggered()), textEdit, SLOT(cut()));

    copyAct = new QAction(/*QIcon(":/images/copy.png"),*/ tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
    connect(copyAct, SIGNAL(triggered()), model, SLOT(copy()));

    pasteAct = new QAction(/*QIcon(":/images/paste.png"),*/ tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
    connect(pasteAct, SIGNAL(triggered()), model, SLOT(paste()));

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    cutAct->setEnabled(false);
    //pasteAct->setEnabled(false);
    //copyAct->setEnabled(false);
    //connect(textEdit, SIGNAL(copyAvailable(bool)),
    //        cutAct, SLOT(setEnabled(bool)));
    //connect(textEdit, SIGNAL(copyAvailable(bool)),
    //        copyAct, SLOT(setEnabled(bool)));
}

//! [implicit tr context]
void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
//! [implicit tr context]
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(closeAct);
    fileMenu->addSeparator();
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
}

void MainWindow::createToolBars()
{
    /*
//! [0]
    fileToolBar = addToolBar(tr("File"));
    fileToolBar->addAction(newAct);
    fileToolBar->addAction(openAct);
//! [0]
    fileToolBar->addAction(saveAct);

    editToolBar = addToolBar(tr("Edit"));
    editToolBar->addAction(cutAct);
    editToolBar->addAction(copyAct);
    editToolBar->addAction(pasteAct);
    */
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Welcome to XaoS"), 5000);
}

void MainWindow::readSettings()
{
    QSettings settings("XaoS Team", "XaoS");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(640, 480)).toSize();
    move(pos);
    resize(size);
}

void MainWindow::writeSettings()
{
    QSettings settings("XaoS Team", "XaoS");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
}

bool MainWindow::maybeSave()
{
    /*
    if (textEdit->document()->isModified()) {
	QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("XaoS"),
                     tr("The document has been modified.\n"
                        "Do you want to save your changes?"),
                     QMessageBox::Save | QMessageBox::Discard
		     | QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
            return save();
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    */
    return true;
}

void MainWindow::loadFile(const QString &fileName)
{

    /*
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("XaoS"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    QTextStream in(&file);
    */

    QApplication::setOverrideCursor(Qt::WaitCursor);
    model->loadFile(fileName);
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    //statusBar()->showMessage(tr("File loaded"), 2000);
}

bool MainWindow::saveFile(const QString &fileName)
{
    /*
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("XaoS"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    */

    QApplication::setOverrideCursor(Qt::WaitCursor);
    model->saveFile(fileName);
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    //statusBar()->showMessage(tr("File saved"), 2000);
    return true;
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    static int sequenceNumber = 1;

    isUntitled = fileName.isEmpty();
    if (isUntitled) {
        curFile = tr("document%1.txt").arg(sequenceNumber++);
    } else {
        curFile = QFileInfo(fileName).canonicalFilePath();
    }

    //textEdit->document()->setModified(false);
    setWindowModified(false);

    setWindowTitle(tr("%1[*] - %2").arg(strippedName(curFile))
                                       .arg(tr("XaoS")));
}

QString MainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

MainWindow *MainWindow::findMainWindow(const QString &fileName)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    foreach (QWidget *widget, qApp->topLevelWidgets()) {
        MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
        if (mainWin && mainWin->curFile == canonicalFilePath)
            return mainWin;
    }
    return 0;
}

void MainWindow::showMessage(const QString &message)
{
    statusBar()->showMessage(message, 5000);
}

void MainWindow::clearMessage()
{
    statusBar()->clearMessage();
}

void MainWindow::showError(const QString &error)
{
    QMessageBox::warning(this, tr("XaoS"), error);
}
