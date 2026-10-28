#pragma once

#include <QPointer>
#include <QWidget>
#include <functional>

// FramelessWindowHint removes the native title bar and border together with
// everything they provide, so moving and resizing is done by hand:
// alt + left mouse button drags the window, dragging its edges resizes it.
// Installs an application-wide event filter and catches mouse events on all
// widgets of the target window.
class FramelessWindowHelper : public QObject {
public:
    explicit FramelessWindowHelper(QWidget *window, std::function<void(bool)> dragEnabledChanged, QObject *parent = nullptr);
    ~FramelessWindowHelper() override;

    // toggles Qt::FramelessWindowHint; returns true if the window flags changed
    bool applyFrameless(bool mode, bool maximized);
    void updateDragEnabled(bool maximized);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    bool framelessActive() const;
    Qt::Edges edgesAt(const QPoint &pos) const;
    void updateCursor(QWidget *widget, Qt::Edges edges);
    void resetCursor();
    void enableMouseTracking();

    QWidget *targetWindow;
    std::function<void(bool)> dragEnabledChanged;
    bool maximizedState;

    enum FramelessDragMode {
        FRAMELESS_DRAG_NONE,
        FRAMELESS_DRAG_MOVE,
        FRAMELESS_DRAG_RESIZE
    };
    FramelessDragMode dragMode;
    Qt::Edges resizeEdges;
    QPoint pressPos;
    QRect startGeometry;
    QPointer<QWidget> cursorWidget;
    Qt::Edges cursorEdges;
};
