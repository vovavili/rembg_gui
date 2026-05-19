#include "MainWindow.h"

#include "ImageViewSync.h"

#include <QAction>
#include <QApplication>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>

namespace rembg::native_app {

MainWindow::MainWindow(bool openOnStart, QWidget* parent)
    : QMainWindow(parent), imageViewSync_(new ImageViewSync(this, this)) {
    setCentralWidget(imageViewSync_);
    createActions(imageViewSync_);
    createMenus();

    setWindowTitle("Rembg");
    resize(1200, 600);

    if (openOnStart) {
        imageViewSync_->openLeft();
    }
}

QAction* MainWindow::printLeftAction() const {
    return printLeftAct_;
}

QAction* MainWindow::printRightAction() const {
    return printRightAct_;
}

QAction* MainWindow::saveRightAction() const {
    return saveRightAct_;
}

QAction* MainWindow::fitToWindowAction() const {
    return fitToWindowAct_;
}

QAction* MainWindow::zoomInAction() const {
    return zoomInAct_;
}

QAction* MainWindow::zoomOutAction() const {
    return zoomOutAct_;
}

QAction* MainWindow::normalSizeAction() const {
    return normalSizeAct_;
}

void MainWindow::fitToWindow() {
    const bool fitToWindow = fitToWindowAct_->isChecked();
    const auto scrollAreas = imageViewSync_->findChildren<PanScrollArea*>();
    for (PanScrollArea* scrollArea : scrollAreas) {
        scrollArea->setWidgetResizable(fitToWindow);
    }

    if (!fitToWindow) {
        imageViewSync_->normalSize();
    }

    imageViewSync_->updateActions();
}

void MainWindow::createActions(ImageViewSync* view) {
    openLeftAct_ = new QAction("&Open Image", this);
    openLeftAct_->setShortcut(QKeySequence::Open);
    connect(openLeftAct_, &QAction::triggered, view, &ImageViewSync::openLeft);

    saveRightAct_ = new QAction("&Save Image", this);
    saveRightAct_->setShortcut(QKeySequence::Save);
    saveRightAct_->setEnabled(false);
    connect(saveRightAct_, &QAction::triggered, view, &ImageViewSync::saveRight);

    printLeftAct_ = new QAction("&Print Left", this);
    printLeftAct_->setShortcut(QKeySequence::Print);
    printLeftAct_->setEnabled(false);
    connect(printLeftAct_, &QAction::triggered, view, &ImageViewSync::printLeft);

    printRightAct_ = new QAction("&Print Right", this);
    printRightAct_->setShortcut(QKeySequence("Shift+Ctrl+P"));
    printRightAct_->setEnabled(false);
    connect(printRightAct_, &QAction::triggered, view, &ImageViewSync::printRight);

    zoomInAct_ = new QAction("Zoom &In (25%)", this);
    zoomInAct_->setShortcut(QKeySequence::ZoomIn);
    zoomInAct_->setEnabled(false);
    connect(zoomInAct_, &QAction::triggered, view, &ImageViewSync::zoomIn);

    zoomOutAct_ = new QAction("Zoom &Out (25%)", this);
    zoomOutAct_->setShortcut(QKeySequence::ZoomOut);
    zoomOutAct_->setEnabled(false);
    connect(zoomOutAct_, &QAction::triggered, view, &ImageViewSync::zoomOut);

    normalSizeAct_ = new QAction("&Normal Size", this);
    normalSizeAct_->setShortcut(QKeySequence("Ctrl+0"));
    normalSizeAct_->setEnabled(false);
    connect(normalSizeAct_, &QAction::triggered, view, &ImageViewSync::normalSize);

    fitToWindowAct_ = new QAction("&Fit to Window", this);
    fitToWindowAct_->setEnabled(false);
    fitToWindowAct_->setCheckable(true);
    fitToWindowAct_->setShortcut(QKeySequence("Ctrl+F"));
    connect(fitToWindowAct_, &QAction::triggered, this, &MainWindow::fitToWindow);

    aboutAct_ = new QAction("&About", this);
    connect(aboutAct_, &QAction::triggered, view, &ImageViewSync::about);

    aboutQtAct_ = new QAction("About &Qt", this);
    connect(aboutQtAct_, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::createMenus() {
    fileMenu_ = new QMenu("&File", this);
    fileMenu_->addAction(openLeftAct_);
    fileMenu_->addAction(saveRightAct_);
    fileMenu_->addSeparator();
    fileMenu_->addAction(printLeftAct_);
    fileMenu_->addAction(printRightAct_);
    fileMenu_->addSeparator();

    viewMenu_ = new QMenu("&View", this);
    viewMenu_->addAction(zoomInAct_);
    viewMenu_->addAction(zoomOutAct_);
    viewMenu_->addAction(normalSizeAct_);
    viewMenu_->addSeparator();
    viewMenu_->addAction(fitToWindowAct_);

    helpMenu_ = new QMenu("&Help", this);
    helpMenu_->addAction(aboutAct_);
    helpMenu_->addAction(aboutQtAct_);

    menuBar()->addMenu(fileMenu_);
    menuBar()->addMenu(viewMenu_);
    menuBar()->addMenu(helpMenu_);
}

}  // namespace rembg::native_app
