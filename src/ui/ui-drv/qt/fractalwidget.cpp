#include "fractalwidget.h"

#include "ui.h"

#include <QtGui>

FractalWidget::FractalWidget()
{
    m_mouseButtons = 0;
    m_mousePosition = QPoint(0, 0);
    m_keyCombination = 0;
    m_activeImage = 0;
    m_image[0] = m_image[1] = 0;

    setFocusPolicy(Qt::WheelFocus);
    setMouseTracking(true);
}

void FractalWidget::updateMouse(QMouseEvent *event)
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

void FractalWidget::mousePressEvent(QMouseEvent *event)
{
    updateMouse(event);
}

void FractalWidget::mouseReleaseEvent(QMouseEvent *event)
{
    updateMouse(event);
}

void FractalWidget::mouseMoveEvent(QMouseEvent *event)
{
    updateMouse(event);
}

void FractalWidget::wheelEvent(QWheelEvent *event)
{
}

void FractalWidget::keyPressEvent(QKeyEvent *event)
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
                else
                    event->ignore();
        }
}

void FractalWidget::keyReleaseEvent(QKeyEvent *event)
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
        default:
                event->ignore();
        }
}

void FractalWidget::resizeEvent(QResizeEvent *event)
{
    if (m_image[0] && m_image[1])
        ui_call_resize();
}

void FractalWidget::paintEvent (QPaintEvent *event)
{
    if (m_image[m_activeImage]) {
        QPainter painter(this);
        painter.drawImage(0, 0, *m_image[m_activeImage]);
    }
}

void FractalWidget::createImages()
{
    m_image[0] = new QImage(width(), height(), QImage::Format_RGB32);
    m_image[1] = new QImage(width(), height(), QImage::Format_RGB32);
    m_activeImage = 0;
}

void FractalWidget::destroyImages()
{
    delete m_image[0];
    delete m_image[1];
}

char *FractalWidget::imageBuffer1()
{
    return (char *)m_image[0]->bits();
}

char *FractalWidget::imageBuffer2()
{
    return (char *)m_image[1]->bits();
}

int FractalWidget::imageBytesPerLine()
{
    return m_image[0]->bytesPerLine();
}

void FractalWidget::switchActiveImage()
{
    m_activeImage ^= 1;
}

QPoint FractalWidget::mousePosition()
{
    return m_mousePosition;
}

int FractalWidget::mouseButtons()
{
    return m_mouseButtons;
}

int FractalWidget::keyCombination()
{
    return m_keyCombination;
}

void FractalWidget::setCursorType(int type)
{
    if (type == WAITMOUSE || type == REPLAYMOUSE)
        setCursor(Qt::WaitCursor);
    else
        setCursor(Qt::ArrowCursor);
}
