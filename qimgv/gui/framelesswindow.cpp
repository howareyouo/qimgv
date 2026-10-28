#include "framelesswindow.h"
#include <QApplication>
#include <QMouseEvent>

static const int FRAMELESS_RESIZE_MARGIN = 6;

static QPoint mouseGlobalPos(const QMouseEvent *event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->globalPosition().toPoint();
#else
    return event->globalPos();
#endif
}

static Qt::CursorShape framelessCursorForEdges(Qt::Edges edges) {
    const bool top = edges & Qt::TopEdge;
    const bool bottom = edges & Qt::BottomEdge;
    const bool left = edges & Qt::LeftEdge;
    const bool horizontal = left || (edges & Qt::RightEdge);
    if((top && left) || (bottom && (edges & Qt::RightEdge)))
        return Qt::SizeFDiagCursor;
    if((top && (edges & Qt::RightEdge)) || (bottom && left))
        return Qt::SizeBDiagCursor;
    if(horizontal)
        return Qt::SizeHorCursor;
    return Qt::SizeVerCursor;
}

FramelessWindowHelper::FramelessWindowHelper(QWidget *_window, std::function<void(bool)> _dragEnabledChanged, QObject *parent)
    : QObject(parent),
      targetWindow(_window),
      dragEnabledChanged(std::move(_dragEnabledChanged)),
      maximizedState(false),
      dragMode(FRAMELESS_DRAG_NONE)
{
    qApp->installEventFilter(this);
}

FramelessWindowHelper::~FramelessWindowHelper() {
    qApp->removeEventFilter(this);
}

bool FramelessWindowHelper::framelessActive() const {
    return targetWindow->windowFlags().testFlag(Qt::FramelessWindowHint)
           && !targetWindow->isFullScreen()
           && !maximizedState;
}

Qt::Edges FramelessWindowHelper::edgesAt(const QPoint &pos) const {
    Qt::Edges edges;
    if(pos.x() < FRAMELESS_RESIZE_MARGIN)
        edges |= Qt::LeftEdge;
    else if(pos.x() > targetWindow->width() - FRAMELESS_RESIZE_MARGIN)
        edges |= Qt::RightEdge;
    if(pos.y() < FRAMELESS_RESIZE_MARGIN)
        edges |= Qt::TopEdge;
    else if(pos.y() > targetWindow->height() - FRAMELESS_RESIZE_MARGIN)
        edges |= Qt::BottomEdge;
    return edges;
}

bool FramelessWindowHelper::applyFrameless(bool mode, bool maximized) {
    if(targetWindow->windowFlags().testFlag(Qt::FramelessWindowHint) == mode)
        return false;
    maximizedState = maximized;
    bool wasVisible = targetWindow->isVisible();
    bool wasFullScreen = targetWindow->isFullScreen();
    targetWindow->setWindowFlag(Qt::FramelessWindowHint, mode);
    // setWindowFlag() hides the window
    if(wasVisible) {
        if(wasFullScreen)
            targetWindow->showFullScreen();
        else if(maximized)
            targetWindow->showMaximized();
        else
            targetWindow->show();
    }
    updateDragEnabled(maximized);
    return true;
}

// The cursor has to be set on the widget under the pointer: most children set
// their own cursor (e.g. the image viewer), which would hide it otherwise
void FramelessWindowHelper::updateCursor(QWidget *widget, Qt::Edges edges) {
    if(cursorWidget.data() == widget && cursorEdges == edges)
        return;
    resetCursor();
    if(!widget || !edges)
        return;
    widget->setCursor(framelessCursorForEdges(edges));
    cursorWidget = widget;
    cursorEdges = edges;
}

void FramelessWindowHelper::resetCursor() {
    if(cursorWidget)
        cursorWidget->unsetCursor();
    cursorWidget = nullptr;
    cursorEdges = Qt::Edges();
}

void FramelessWindowHelper::enableMouseTracking() {
    const QList<QWidget*> widgets = targetWindow->findChildren<QWidget*>();
    for(QWidget *widget : widgets) {
        if(!widget->hasMouseTracking())
            widget->setMouseTracking(true);
    }
}

void FramelessWindowHelper::updateDragEnabled(bool maximized) {
    maximizedState = maximized;
    const bool frameless = framelessActive();
    if(dragEnabledChanged)
        dragEnabledChanged(frameless);
    if(!frameless) {
        resetCursor();
        return;
    }
    // child widgets only report hovering with mouse tracking enabled, and the
    // cursor is what tells the user that the edges can be dragged
    enableMouseTracking();
}

bool FramelessWindowHelper::eventFilter(QObject *obj, QEvent *event) {
    const QEvent::Type type = event->type();
    if(type == QEvent::Leave) {
        // do not leave the resize cursor behind when the pointer leaves
        if(cursorWidget && cursorWidget.data() == qobject_cast<QWidget*>(obj))
            resetCursor();
        return false;
    }
    if(type != QEvent::MouseButtonPress
       && type != QEvent::MouseMove
       && type != QEvent::MouseButtonRelease)
        return false;
    if(!framelessActive())
        return false;
    QWidget *widget = qobject_cast<QWidget*>(obj);
    if(!widget || widget->window() != targetWindow)
        return false;

    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

    if(type == QEvent::MouseButtonPress) {
        if(mouseEvent->button() != Qt::LeftButton)
            return false;
        if(mouseEvent->modifiers() == Qt::AltModifier) {
            dragMode = FRAMELESS_DRAG_MOVE;
        } else {
            Qt::Edges edges = edgesAt(targetWindow->mapFromGlobal(mouseGlobalPos(mouseEvent)));
            if(!edges)
                return false;
            dragMode = FRAMELESS_DRAG_RESIZE;
            resizeEdges = edges;
            updateCursor(widget, edges);
        }
        pressPos = mouseGlobalPos(mouseEvent);
        startGeometry = targetWindow->geometry();
        // do not pass the click to the widget under the cursor
        return true;
    }

    if(type == QEvent::MouseMove && !mouseEvent->buttons()) {
        // hovering: show which edge would be dragged
        updateCursor(widget, edgesAt(targetWindow->mapFromGlobal(mouseGlobalPos(mouseEvent))));
        return false;
    }

    if(dragMode == FRAMELESS_DRAG_NONE)
        return false;

    if(type == QEvent::MouseButtonRelease) {
        dragMode = FRAMELESS_DRAG_NONE;
        return true;
    }

    if(!(mouseEvent->buttons() & Qt::LeftButton)) {
        dragMode = FRAMELESS_DRAG_NONE;
        return false;
    }

    QPoint delta = mouseGlobalPos(mouseEvent) - pressPos;
    if(dragMode == FRAMELESS_DRAG_MOVE) {
        targetWindow->move(startGeometry.topLeft() + delta);
    } else {
        QRect newGeometry = startGeometry;
        const int minWidth = qMax(targetWindow->minimumWidth(), 100);
        const int minHeight = qMax(targetWindow->minimumHeight(), 100);
        if(resizeEdges & Qt::LeftEdge)
            newGeometry.setLeft(qMin(newGeometry.left() + delta.x(), newGeometry.right() - minWidth));
        else if(resizeEdges & Qt::RightEdge)
            newGeometry.setRight(qMax(newGeometry.right() + delta.x(), newGeometry.left() + minWidth));
        if(resizeEdges & Qt::TopEdge)
            newGeometry.setTop(qMin(newGeometry.top() + delta.y(), newGeometry.bottom() - minHeight));
        else if(resizeEdges & Qt::BottomEdge)
            newGeometry.setBottom(qMax(newGeometry.bottom() + delta.y(), newGeometry.top() + minHeight));
        targetWindow->setGeometry(newGeometry);
    }
    return true;
}
