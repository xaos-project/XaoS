#include "mainwindow.h"
#include "fractalwidget.h"
#include "customdialog.h"

#include "ui.h"

#include <QtGui>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_mouseButtons = 0;
    m_mousePosition = QPoint(0, 0);
    m_keyCombination = 0;
    m_image[0] = m_image[1] = 0;
    m_uih = 0;

    setWindowTitle(QCoreApplication::applicationName());
    setWindowIcon(QIcon(":/images/xaosbig.png"));

    m_fractalWidget = new FractalWidget();
    setCentralWidget(m_fractalWidget);
    connect(m_fractalWidget, SIGNAL(mouseChanged(QMouseEvent*)), this, SLOT(updateMouse(QMouseEvent*)));
    connect(m_fractalWidget, SIGNAL(sizeChanged()), this, SLOT(updateSize()));
    connect(m_fractalWidget, SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(addKey(QKeyEvent*)));
    connect(m_fractalWidget, SIGNAL(keyReleased(QKeyEvent*)), this, SLOT(removeKey(QKeyEvent*)));

    readSettings();
}

MainWindow::~MainWindow()
{
}

void MainWindow::readSettings()
{
    QSettings settings;
    QPoint pos = settings.value("windowPosition", QPoint(200, 200)).toPoint();
    QSize size = settings.value("imageSize", QSize(640, 480)).toSize();
    resize(size);
    move(pos);
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue("windowPosition", pos());
    settings.setValue("imageSize", size());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    ui_quit();
}

void MainWindow::updateMouse(QMouseEvent *event)
{
    m_mousePosition = event->pos();
    m_mouseButtons = 0;
    if (event->buttons() & Qt::LeftButton)
    {
        // Use modifier keys to emulate other buttons
#ifdef Q_WS_MAC
        // Qt::MetaModifier maps to control key on Macs
        if (event->modifiers() & Qt::MetaModifier)
#else
        if (event->modifiers() & Qt::ControlModifier)
#endif
            m_mouseButtons |= BUTTON3;
        else if (event->modifiers() & Qt::ShiftModifier)
            m_mouseButtons |= BUTTON2;
        else
            m_mouseButtons |= BUTTON1;
    }
    if (event->buttons() & Qt::MidButton)
        m_mouseButtons |= BUTTON2;
    if (event->buttons() & Qt::RightButton)
        m_mouseButtons |= BUTTON3;
}

void MainWindow::updateMouse(QWheelEvent *event)
{
}

void MainWindow::addKey(QKeyEvent *event)
{
        switch (event->key()) {
        case Qt::Key_Left:
                m_keyCombination |= 1;
                ui_key(UIKEY_LEFT);
                break;
        case Qt::Key_Right:
                m_keyCombination |= 2;
                ui_key(UIKEY_RIGHT);
                break;
        case Qt::Key_Up:
                m_keyCombination |= 4;
                ui_key(UIKEY_UP);
                break;
        case Qt::Key_Down:
                m_keyCombination |= 8;
                ui_key(UIKEY_DOWN);
                break;
        case Qt::Key_PageUp:
                ui_key(UIKEY_PGUP);
                break;
        case Qt::Key_PageDown:
                ui_key(UIKEY_PGDOWN);
                break;
        case Qt::Key_Backspace:
                ui_key(UIKEY_BACKSPACE);
                break;
        case Qt::Key_Escape:
                ui_key(UIKEY_ESC);
                break;
        case Qt::Key_Home:
                ui_key(UIKEY_HOME);
                break;
        case Qt::Key_End:
                ui_key(UIKEY_END);
                break;
        case Qt::Key_Tab:
                ui_key(UIKEY_TAB);
                break;
        default:
                if (!event->text().isEmpty())
                    ui_key(event->text().toAscii()[0]);
        }
}

void MainWindow::removeKey(QKeyEvent *event)
{
        switch (event->key()) {
        case Qt::Key_Left:
                m_keyCombination &= ~1;
                break;
        case Qt::Key_Right:
                m_keyCombination &= ~2;
                break;
        case Qt::Key_Up:
                m_keyCombination &= ~4;
                break;
        case Qt::Key_Down:
                m_keyCombination &= ~8;
                break;
        }
}

void MainWindow::updateSize()
{
    if (m_image[0] && m_image[1])
        ui_resize();
}

void MainWindow::createImages()
{
    m_image[0] = new QImage(m_fractalWidget->width(),
                            m_fractalWidget->height(),
                            QImage::Format_RGB32);
    m_image[1] = new QImage(m_fractalWidget->width(),
                            m_fractalWidget->height(),
                            QImage::Format_RGB32);
    m_activeImage = 0;
}

void MainWindow::destroyImages()
{
    delete m_image[0];
    delete m_image[1];
}

char *MainWindow::imageBuffer1()
{
    return (char *)m_image[0]->bits();
}

char *MainWindow::imageBuffer2()
{
    return (char *)m_image[1]->bits();
}

int MainWindow::imageBytesPerLine()
{
    return m_image[0]->bytesPerLine();
}

QSize MainWindow::imageSize()
{
    return m_fractalWidget->size();
}

void MainWindow::switchActiveImage()
{
    m_activeImage ^= 1;
}

void MainWindow::redrawImage()
{
    m_fractalWidget->drawImage(m_image[m_activeImage]);
}

QPoint MainWindow::mousePosition()
{
    return m_mousePosition;
}

int MainWindow::mouseButtons()
{
    return m_mouseButtons;
}

int MainWindow::keyCombination()
{
    return m_keyCombination;
}

void MainWindow::showMessage(const QString &message)
{
    if (!message.isEmpty())
        statusBar()->showMessage(message, 5000);
}

void MainWindow::showError(const QString &error)
{
    if (!error.isEmpty())
        QMessageBox::warning(this, this->windowTitle(), error, QMessageBox::Close);
}

void MainWindow::setCursorType(int type)
{
    if (type == WAITMOUSE || type == REPLAYMOUSE)
        m_fractalWidget->setCursor(Qt::WaitCursor);
    else
        m_fractalWidget->setCursor(Qt::ArrowCursor);
}

void MainWindow::buildMenu(struct uih_context *uih, const char *name)
{
    m_uih = uih;

    menuBar()->clear();

    const menuitem *item;
    for (int i = 0; (item = menu_item(name, i)) != NULL; i++) {
        if (item->type == MENU_SUBMENU) {
            QMenu *menu = menuBar()->addMenu(QString(item->name));
            buildMenu(uih, item->shortname, menu);
        }
    }
}

QKeySequence::StandardKey MainWindow::keyForItem(const QString &name)
{
    static QHash<QString, QKeySequence::StandardKey> map;
    if (map.isEmpty()) {
        map["initstate"] = QKeySequence::New;
        map["loadpos"] = QKeySequence::Open;
        map["savepos"] = QKeySequence::Save;
        map["undo"] = QKeySequence::Undo;
        map["redo"] = QKeySequence::Redo;
        map["recalculate"] = QKeySequence::Refresh;
        map["help"] = QKeySequence::HelpContents;
    }

    if (map.contains(name)) {
        return map[name];
    } else {
        return QKeySequence::UnknownKey;
    }
}

void MainWindow::buildMenu(struct uih_context *uih, const char *name, QMenu *parent)
{
    m_uih = uih;

    QActionGroup *group = new QActionGroup(parent);

    const menuitem *item;
    for (int i = 0; (item = menu_item(name, i)) != NULL; i++) {

        QString itemName(item->name);
        if (item->type == MENU_DIALOG || item->type == MENU_CUSTOMDIALOG)
            itemName += "...";

        if (item->type == MENU_SEPARATOR) {
            parent->addSeparator();
        } else if (item->type == MENU_SUBMENU) {
            QMenu *menu = parent->addMenu(item->name);
            connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
            buildMenu(uih, item->shortname, menu);
        } else {
            QAction *action = new QAction(itemName, parent);
            action->setShortcuts(keyForItem(item->shortname));
            action->setObjectName(item->shortname);
            if (item->flags & (MENUFLAG_RADIO | MENUFLAG_CHECKBOX)) {
                action->setCheckable(true);
                action->setChecked(menu_enabled(item, uih));
                if (item->flags & MENUFLAG_RADIO)
                    action->setActionGroup(group);
            }
            connect(action, SIGNAL(triggered()), this, SLOT(activateMenuItem()));
            parent->addAction(action);
        }
    }
}

void MainWindow::updateMenu()
{
    QMenu *menu = qobject_cast<QMenu *>(sender());
    QAction *action;
    foreach(action, menu->actions()) {
        if (action->isCheckable()) {
            const menuitem *item = menu_findcommand(action->objectName().toAscii());
            action->setChecked(menu_enabled(item, m_uih));
        }
    }
}

void MainWindow::activateMenuItem()
{
    QAction *action = qobject_cast<QAction *>(sender());
    const menuitem *item = menu_findcommand(action->objectName().toAscii());
    ui_menuactivate(item, NULL);
}

void MainWindow::showDialog(struct uih_context *uih, const char *name)
{
    m_uih = uih;

    const menuitem *item = menu_findcommand(name);
    if (!item) return;

    const menudialog *dialog = menu_getdialog(uih, item);
    if (!dialog) return;

    int nitems;
    for (nitems = 0; dialog[nitems].question; nitems++);

    if (nitems == 1 && (dialog[0].type == DIALOG_IFILE || dialog[0].type == DIALOG_OFILE)) {
        QString filter = QString("*.%1").arg(QFileInfo(dialog[0].defstr).completeSuffix());
        QString directory;// = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);

        QString fileName;
        if (dialog[0].type == DIALOG_IFILE)
            fileName = QFileDialog::getOpenFileName(this, item->name, directory, filter);
        else if (dialog[0].type == DIALOG_OFILE)
            fileName = QFileDialog::getSaveFileName(this, item->name, directory, filter);


        if (!fileName.isNull()) {
            dialogparam *param = (dialogparam *)malloc(sizeof(dialogparam));
            param->dstring = strdup(fileName.toUtf8());
            ui_menuactivate(item, param);
        }
    } else {
        CustomDialog customDialog(uih, item, dialog, this);
        if (customDialog.exec() == QDialog::Accepted)
            ui_menuactivate(item, customDialog.parameters());
    }
}
