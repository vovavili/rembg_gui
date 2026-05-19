#include "ImageViewSync.h"

#include "MainWindow.h"

#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImageReader>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPrintDialog>
#include <QScrollBar>
#include <QSizePolicy>

namespace rembg::native_app {

ImageViewSync::ImageViewSync(MainWindow* window, QWidget* parent)
    : QWidget(parent),
      window_(window),
      imageLabelLeft_(new QLabel(this)),
      imageLabelRight_(new QLabel(this)),
      scrollAreaLeft_(new PanScrollArea(this)),
      scrollAreaRight_(new PanScrollArea(this)) {
    imageLabelLeft_->setBackgroundRole(QPalette::Base);
    imageLabelLeft_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabelLeft_->setScaledContents(true);

    scrollAreaLeft_->setBackgroundRole(QPalette::Dark);
    scrollAreaLeft_->setWidget(imageLabelLeft_);
    scrollAreaLeft_->setVisible(false);

    imageLabelRight_->setBackgroundRole(QPalette::Base);
    imageLabelRight_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabelRight_->setScaledContents(true);
    imageLabelRight_->setAlignment(Qt::AlignCenter);

    scrollAreaRight_->setBackgroundRole(QPalette::Dark);
    scrollAreaRight_->setWidget(imageLabelRight_);
    scrollAreaRight_->setVisible(false);

    auto* layout = new QHBoxLayout(this);
    layout->addWidget(scrollAreaLeft_);
    layout->addWidget(scrollAreaRight_);
    setLayout(layout);

    connect(
        scrollAreaLeft_->verticalScrollBar(),
        &QScrollBar::valueChanged,
        scrollAreaRight_->verticalScrollBar(),
        &QScrollBar::setValue
    );
    connect(
        scrollAreaLeft_->horizontalScrollBar(),
        &QScrollBar::valueChanged,
        scrollAreaRight_->horizontalScrollBar(),
        &QScrollBar::setValue
    );
    connect(
        scrollAreaRight_->verticalScrollBar(),
        &QScrollBar::valueChanged,
        scrollAreaLeft_->verticalScrollBar(),
        &QScrollBar::setValue
    );
    connect(
        scrollAreaRight_->horizontalScrollBar(),
        &QScrollBar::valueChanged,
        scrollAreaLeft_->horizontalScrollBar(),
        &QScrollBar::setValue
    );

    worker_ = new RemovalWorker();
    worker_->moveToThread(&workerThread_);
    connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
    connect(this, &ImageViewSync::removalRequested, worker_, &RemovalWorker::process);
    connect(worker_, &RemovalWorker::statusChanged, this, &ImageViewSync::onRemovalStatus);
    connect(worker_, &RemovalWorker::progressChanged, this, &ImageViewSync::onRemovalProgress);
    connect(worker_, &RemovalWorker::finished, this, &ImageViewSync::onRemovalFinished);
    connect(worker_, &RemovalWorker::failed, this, &ImageViewSync::onRemovalFailed);
    workerThread_.start();
}

ImageViewSync::~ImageViewSync() {
    workerThread_.quit();
    workerThread_.wait();
}

void ImageViewSync::openLeft() {
    const QString selectedFileName = QFileDialog::getOpenFileName(
        this,
        "QFileDialog.getOpenFileName()",
        QString(),
        "Images (*.png *.jpeg *.jpg *.bmp *.gif)"
    );

    if (selectedFileName.isEmpty()) {
        return;
    }

    QString errorMessage;
    QImage image = loadImage(selectedFileName, &errorMessage);
    if (image.isNull()) {
        QMessageBox::information(this, "Image Viewer", errorMessage);
        return;
    }

    fileName_ = selectedFileName;
    sourceImage_ = image;
    rightImage_ = QImage();
    rightMask_ = QImage();
    scaleFactor_ = 1.0;

    setLeftImage(sourceImage_);
    setRightStatus("Preparing model...");
    scrollAreaLeft_->setVisible(true);
    scrollAreaRight_->setVisible(true);

    window_->printLeftAction()->setEnabled(true);
    window_->printRightAction()->setEnabled(false);
    window_->saveRightAction()->setEnabled(false);
    window_->fitToWindowAction()->setEnabled(true);
    updateActions();

    if (!window_->fitToWindowAction()->isChecked()) {
        imageLabelLeft_->adjustSize();
        imageLabelRight_->adjustSize();
    }

    emit removalRequested(fileName_, sourceImage_);
}

void ImageViewSync::saveRight() {
    if (rightImage_.isNull() || fileName_.isEmpty()) {
        QMessageBox::information(this, "Image Viewer", "There is no processed image to save.");
        return;
    }

    const QFileInfo inputFile(fileName_);
    const QString outputFileName =
        inputFile.absolutePath() + "/" + inputFile.completeBaseName() + "_no_bg.png";

    if (!rightImage_.save(outputFileName, "PNG")) {
        QMessageBox::information(this, "Image Viewer", "Cannot save " + outputFileName + ".");
        return;
    }

    QMessageBox::information(
        this,
        "Image Viewer",
        "Image saved - " + QFileInfo(outputFileName).fileName() + "."
    );
}

void ImageViewSync::printLeft() {
    if (const QPixmap pixmap = imageLabelLeft_->pixmap(); !pixmap.isNull()) {
        printImage(pixmap);
    }
}

void ImageViewSync::printRight() {
    if (const QPixmap pixmap = imageLabelRight_->pixmap(); !pixmap.isNull()) {
        printImage(pixmap);
    }
}

void ImageViewSync::zoomIn() {
    scaleImage(1.25);
}

void ImageViewSync::zoomOut() {
    scaleImage(0.8);
}

void ImageViewSync::normalSize() {
    imageLabelLeft_->adjustSize();
    imageLabelRight_->adjustSize();
    scaleFactor_ = 1.0;
}

void ImageViewSync::about() {
    QMessageBox::about(
        this,
        "Image View in the Main Window",
        "<p>The <b>Image Viewer</b> example shows how to combine QLabel and QScrollArea "
        "to display an image. QLabel is typically used for displaying text, but it can "
        "also display an image. QScrollArea provides a scrolling view around another "
        "widget.</p><p>This C++ build keeps the original two-pane viewer and uses "
        "ONNX Runtime directly for background removal.</p>"
    );
}

void ImageViewSync::updateActions() {
    const bool canZoom =
        !window_->fitToWindowAction()->isChecked() && !sourceImage_.isNull();
    window_->zoomInAction()->setEnabled(canZoom && scaleFactor_ < 3.0);
    window_->zoomOutAction()->setEnabled(canZoom && scaleFactor_ > 0.333);
    window_->normalSizeAction()->setEnabled(canZoom);
}

void ImageViewSync::onRemovalStatus(const QString& fileName, const QString& message) {
    if (fileName == fileName_) {
        setRightStatus(message);
    }
}

void ImageViewSync::onRemovalProgress(
    const QString& fileName,
    qint64 bytesReceived,
    qint64 bytesTotal
) {
    if (fileName != fileName_) {
        return;
    }

    if (bytesTotal > 0) {
        const int percent = static_cast<int>((bytesReceived * 100) / bytesTotal);
        setRightStatus(QStringLiteral("Downloading model... %1%").arg(percent));
    } else {
        setRightStatus("Downloading model...");
    }
}

void ImageViewSync::onRemovalFinished(
    const QString& fileName,
    const QImage& output,
    const QImage& mask
) {
    if (fileName != fileName_) {
        return;
    }

    rightImage_ = output;
    rightMask_ = mask;
    setRightImage(rightImage_);

    window_->printRightAction()->setEnabled(true);
    window_->saveRightAction()->setEnabled(true);
    updateActions();

    if (!window_->fitToWindowAction()->isChecked()) {
        imageLabelRight_->adjustSize();
    }
}

void ImageViewSync::onRemovalFailed(const QString& fileName, const QString& message) {
    if (fileName != fileName_) {
        return;
    }

    setRightStatus("Background removal failed.");
    QMessageBox::information(this, "Image Viewer", message);
    window_->printRightAction()->setEnabled(false);
    window_->saveRightAction()->setEnabled(false);
    updateActions();
}

void ImageViewSync::setLeftImage(const QImage& image) {
    imageLabelLeft_->setPixmap(QPixmap::fromImage(image));
}

void ImageViewSync::setRightImage(const QImage& image) {
    imageLabelRight_->setText(QString());
    imageLabelRight_->setPixmap(QPixmap::fromImage(image));
}

void ImageViewSync::setRightStatus(const QString& message) {
    imageLabelRight_->clear();
    imageLabelRight_->setText(message);
    imageLabelRight_->setAlignment(Qt::AlignCenter);
    imageLabelRight_->setMinimumSize(imageLabelLeft_->sizeHint());
}

void ImageViewSync::scaleImage(double factor) {
    const QPixmap leftPixmap = imageLabelLeft_->pixmap();
    const QPixmap rightPixmap = imageLabelRight_->pixmap();
    if (leftPixmap.isNull()) {
        return;
    }

    scaleFactor_ *= factor;
    imageLabelLeft_->resize(scaleFactor_ * leftPixmap.size());
    if (!rightPixmap.isNull()) {
        imageLabelRight_->resize(scaleFactor_ * rightPixmap.size());
    }

    adjustScrollBar(scrollAreaLeft_->horizontalScrollBar(), factor);
    adjustScrollBar(scrollAreaLeft_->verticalScrollBar(), factor);
    adjustScrollBar(scrollAreaRight_->horizontalScrollBar(), factor);
    adjustScrollBar(scrollAreaRight_->verticalScrollBar(), factor);

    updateActions();
}

void ImageViewSync::adjustScrollBar(QScrollBar* scrollBar, double factor) {
    scrollBar->setValue(
        static_cast<int>(factor * scrollBar->value() + ((factor - 1) * scrollBar->pageStep() / 2))
    );
}

QImage ImageViewSync::loadImage(const QString& fileName, QString* errorMessage) {
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    QImage image = reader.read();
    if (image.isNull() && errorMessage != nullptr) {
        const QString reason =
            reader.errorString().isEmpty() ? QStringLiteral("Unsupported image format.") : reader.errorString();
        *errorMessage =
            QStringLiteral("Cannot load %1.\n\n%2").arg(QDir::toNativeSeparators(fileName), reason);
    }
    return image;
}

void ImageViewSync::printImage(const QPixmap& pixmap) {
    QPrintDialog dialog(&printer_, this);
    if (dialog.exec() == QDialog::Accepted) {
        QPainter painter(&printer_);
        QRect rect = painter.viewport();
        QSize size = pixmap.size();
        size.scale(rect.size(), Qt::KeepAspectRatio);
        painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
        painter.setWindow(pixmap.rect());
        painter.drawPixmap(0, 0, pixmap);
    }
}

}  // namespace rembg::native_app
