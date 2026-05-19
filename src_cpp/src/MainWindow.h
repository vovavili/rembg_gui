#pragma once

#include <QMainWindow>

class QAction;
class QMenu;

namespace rembg::native_app {

class ImageViewSync;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(bool openOnStart = true, QWidget* parent = nullptr);

    QAction* printLeftAction() const;
    QAction* printRightAction() const;
    QAction* saveRightAction() const;
    QAction* fitToWindowAction() const;
    QAction* zoomInAction() const;
    QAction* zoomOutAction() const;
    QAction* normalSizeAction() const;

public slots:
    void fitToWindow();

private:
    void createActions(ImageViewSync* view);
    void createMenus();

    ImageViewSync* imageViewSync_ = nullptr;

    QAction* openLeftAct_ = nullptr;
    QAction* saveRightAct_ = nullptr;
    QAction* printLeftAct_ = nullptr;
    QAction* printRightAct_ = nullptr;
    QAction* zoomInAct_ = nullptr;
    QAction* zoomOutAct_ = nullptr;
    QAction* normalSizeAct_ = nullptr;
    QAction* fitToWindowAct_ = nullptr;
    QAction* aboutAct_ = nullptr;
    QAction* aboutQtAct_ = nullptr;

    QMenu* fileMenu_ = nullptr;
    QMenu* viewMenu_ = nullptr;
    QMenu* helpMenu_ = nullptr;
};

}  // namespace rembg::native_app
