#pragma once

#include "PanScrollArea.h"
#include "RemovalWorker.h"

#include <memory>

#include <QImage>
#include <QLabel>
#include <QPrinter>
#include <QThread>
#include <QWidget>

class QAction;
class QScrollBar;

namespace rembg::native_app {

class MainWindow;

class ImageViewSync : public QWidget {
    Q_OBJECT

public:
    explicit ImageViewSync(MainWindow* window, QWidget* parent = nullptr);
    ~ImageViewSync() override;

public slots:
    void openLeft();
    void saveRight();
    void printLeft();
    void printRight();
    void zoomIn();
    void zoomOut();
    void normalSize();
    void about();
    void updateActions();

signals:
    void removalRequested(const QString& fileName, const QImage& image);

private slots:
    void onRemovalStatus(const QString& fileName, const QString& message);
    void onRemovalProgress(const QString& fileName, qint64 bytesReceived, qint64 bytesTotal);
    void onRemovalFinished(const QString& fileName, const QImage& output, const QImage& mask);
    void onRemovalFailed(const QString& fileName, const QString& message);

private:
    void setLeftImage(const QImage& image);
    void setRightImage(const QImage& image);
    void setRightStatus(const QString& message);
    void scaleImage(double factor);
    static void adjustScrollBar(QScrollBar* scrollBar, double factor);
    static QImage loadImage(const QString& fileName, QString* errorMessage);
    void printImage(const QPixmap& pixmap);

    MainWindow* window_;
    QPrinter printer_;
    double scaleFactor_ = 0.0;
    QString fileName_;
    QImage sourceImage_;
    QImage rightImage_;
    QImage rightMask_;

    QLabel* imageLabelLeft_;
    QLabel* imageLabelRight_;
    PanScrollArea* scrollAreaLeft_;
    PanScrollArea* scrollAreaRight_;

    QThread workerThread_;
    RemovalWorker* worker_ = nullptr;
};

}  // namespace rembg::native_app
