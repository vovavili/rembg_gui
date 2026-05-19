#pragma once

#include <QPoint>
#include <QScrollArea>

class QMouseEvent;

namespace rembg::native_app {

class PanScrollArea : public QScrollArea {
    Q_OBJECT

public:
    explicit PanScrollArea(QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool pressed_ = false;
    QPoint initialMousePosition_;
    int initialHorizontalValue_ = 0;
    int initialVerticalValue_ = 0;
};

}  // namespace rembg::native_app
