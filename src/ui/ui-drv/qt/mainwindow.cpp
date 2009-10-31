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

    m_image[0] = m_image[1] = 0;

    m_fractalWidget = new FractalWidget();
    setCentralWidget(m_fractalWidget);
    connect(m_fractalWidget, SIGNAL(mouseChanged(QMouseEvent*)), this, SLOT(updateMouse(QMouseEvent*)));
    connect(m_fractalWidget, SIGNAL(sizeChanged()), this, SLOT(updateSize()));

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

void MainWindow::updateMouse(QMouseEvent *event)
{
    m_mousePosition = event->pos();
    m_mouseButtons = 0;
    if (event->buttons() & Qt::LeftButton)
        m_mouseButtons |= BUTTON1;
    if (event->buttons() & Qt::MidButton)
        m_mouseButtons |= BUTTON2;
    if (event->buttons() & Qt::RightButton)
        m_mouseButtons |= BUTTON3;
}

void MainWindow::updateMouse(QWheelEvent *event)
{
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

void MainWindow::showMessage(const QString &message)
{
    statusBar()->showMessage(message, 5000);
}

void MainWindow::setCursorType(int type)
{
    switch (type) {
    case WAITMOUSE:
    case REPLAYMOUSE:
        m_fractalWidget->setCursor(Qt::WaitCursor);
        break;

    case NORMALMOUSE:
    default:
        m_fractalWidget->setCursor(Qt::ArrowCursor);
        break;
    }
}

void MainWindow::startMainLoop()
{
    ui_mainloop(0);
    QTimer::singleShot(0, this, SLOT(startMainLoop()));
}

void MainWindow::buildMenu(struct uih_context *uih, const char *name)
{
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
    QActionGroup *group = new QActionGroup(this);

    const menuitem *item;
    for (int i = 0; (item = menu_item(name, i)) != NULL; i++) {
        QString itemName(item->name);
        switch (item->type) {
        case MENU_SEPARATOR:
            parent->addSeparator();
            break;

        case MENU_SUBMENU:
        {
            QMenu *menu = parent->addMenu(item->name);
            buildMenu(uih, item->shortname, menu);
            break;
        }

        case MENU_DIALOG:
        case MENU_CUSTOMDIALOG:
            itemName += "...";
        default:
        {
            QAction *action = new QAction(itemName, this);
            action->setShortcuts(keyForItem(item->shortname));
            action->setData(QVariant(item->shortname));
            if (item->flags & (MENUFLAG_RADIO | MENUFLAG_CHECKBOX)) {
                action->setCheckable(true);
                action->setChecked(menu_enabled(item, uih));
                if (item->flags & MENUFLAG_RADIO)
                    action->setActionGroup(group);
            }
            connect(action, SIGNAL(triggered()), this, SLOT(activateMenuItem()));
            parent->addAction(action);
            break;
        }
        }
    }
}

void MainWindow::activateMenuItem()
{
    QAction *action = qobject_cast<QAction *>(sender());
    const menuitem *item = menu_findcommand(action->data().toString().toAscii());
    ui_menuactivate(item, NULL);
}

void MainWindow::showDialog(struct uih_context *uih, const char *name)
{
    const menuitem *item = menu_findcommand(name);
    if (!item) return;

    const menudialog *dialog = menu_getdialog(uih, item);
    if (!dialog) return;

    int nitems;
    for (nitems = 0; dialog[nitems].question; nitems++);

    if (nitems == 1 && (dialog[0].type == DIALOG_IFILE || dialog[0].type == DIALOG_OFILE)) {
        QString fileName;

        if (dialog[0].type == DIALOG_IFILE)
             fileName = QFileDialog::getOpenFileName(this);
        else if (dialog[0].type == DIALOG_OFILE)
             fileName = QFileDialog::getSaveFileName(this);

        if (!fileName.isNull()) {
            dialogparam *param = (dialogparam *)malloc(sizeof (dialogparam));
            param->dstring = strdup(fileName.toUtf8());
            ui_menuactivate(item, param);
        }
    } else {
        CustomDialog customDialog(uih, item, dialog, this);
        customDialog.exec();
    }
}
