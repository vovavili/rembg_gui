#include "PanScrollArea.h"

#include <QMouseEvent>
#include <QScrollBar>

namespace rembg::native_app {

PanScrollArea::PanScrollArea(QWidget* parent) : QScrollArea(parent) {
    setCursor(Qt::OpenHandCursor);
}

void PanScrollArea::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        pressed_ = true;
        setCursor(Qt::ClosedHandCursor);
        initialMousePosition_ = event->position().toPoint();
        initialHorizontalValue_ = horizontalScrollBar()->value();
        initialVerticalValue_ = verticalScrollBar()->value();
        event->accept();
        return;
    }

    QScrollArea::mousePressEvent(event);
}

void PanScrollArea::mouseMoveEvent(QMouseEvent* event) {
    if (pressed_) {
        const QPoint delta = event->position().toPoint() - initialMousePosition_;
        horizontalScrollBar()->setValue(initialHorizontalValue_ - delta.x());
        verticalScrollBar()->setValue(initialVerticalValue_ - delta.y());
        event->accept();
        return;
    }

    QScrollArea::mouseMoveEvent(event);
}

void PanScrollArea::mouseReleaseEvent(QMouseEvent* event) {
    if (pressed_ && event->button() == Qt::LeftButton) {
        pressed_ = false;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }

    QScrollArea::mouseReleaseEvent(event);
}

}  // namespace rembg::native_app
