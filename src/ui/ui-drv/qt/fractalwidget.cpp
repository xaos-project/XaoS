#include "fractalwidget.h"

#include "ui.h"

#include <QtGui>
#include <QtOpenGL>

FractalWidget::FractalWidget()
{
    m_mouseButtons = 0;
    m_mousePosition = QPoint(0, 0);
    m_keyCombination = 0;
    m_activeImage = 0;
    m_image[0] = m_image[1] = 0;

    setAutoFillBackground(false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_PaintOnScreen, true);

    setFocusPolicy(Qt::WheelFocus);
    setMouseTracking(true);
}

void FractalWidget::updateMouse(QMouseEvent *event)
{
    m_mousePosition = event->pos();
    m_mouseButtons = event->buttons();
    m_keyboardModifiers = event->modifiers();
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
    m_keyboardModifiers = event->modifiers();

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
            ui_key(event->text().toLatin1()[0]);
        else
            event->ignore();
    }
}

void FractalWidget::keyReleaseEvent(QKeyEvent *event)
{
    m_keyboardModifiers = event->modifiers();

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

#ifdef USE_OPENGL
void FractalWidget::paintGL()
{
    if (m_image[m_activeImage]) {
        QImage glimage = QGLWidget::convertToGLFormat(*m_image[m_activeImage]);
        glDrawPixels(glimage.width(), glimage.height(), GL_RGBA, GL_UNSIGNED_BYTE, glimage.bits());
    }
}

void FractalWidget::resizeGL(int w, int h)
{
    glViewport (0, 0, w, h);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w,0,h,-1,1);
    glMatrixMode (GL_MODELVIEW);
}
#else
void FractalWidget::paintEvent (QPaintEvent *event)
{
    if (m_image[m_activeImage]) {
        QPainter painter(this);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.drawImage(0, 0, *m_image[m_activeImage]);
    }
}
#endif

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

void *FractalWidget::imagePointer()
{
    return m_image;
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

   // Qt::MetaModifier maps to control key on Macs
   Qt::KeyboardModifier controlModifier =
#ifdef Q_WS_MAC
           Qt::MetaModifier;
#else
           Qt::ControlModifier;
#endif

    int mouseButtons = 0;

    // Modifier keys change behavior of left and right mouse buttons
    if (m_keyboardModifiers & controlModifier) {
        // Control key swaps left and right buttons
        if (m_mouseButtons & Qt::LeftButton)
            mouseButtons |= BUTTON3;
        if (m_mouseButtons & Qt::RightButton)
            mouseButtons |= BUTTON1;
    } else if (m_keyboardModifiers & Qt::ShiftModifier) {
        // Shift key makes left and right buttons emulate middle button
        mouseButtons |= BUTTON2;
    } else {
        // Otherwise, mouse buttons map normally
        if (m_mouseButtons & Qt::LeftButton)
            mouseButtons |= BUTTON1;
        if (m_mouseButtons & Qt::RightButton)
            mouseButtons |= BUTTON3;
    }

    // Middle button is unaffected by modifier keys
    if (m_mouseButtons & Qt::MidButton)
        mouseButtons |= BUTTON2;

    return mouseButtons;
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

QSize FractalWidget::sizeHint() const
{
    return m_sizeHint;
}

void FractalWidget::setSizeHint(const QSize &size)
{
    m_sizeHint = size;
}
