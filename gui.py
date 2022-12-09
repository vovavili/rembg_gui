#!/usr/bin/python3
"""
A simple PyQT5 GUI for rembg, a tool to remove images background.
"""

import sys
from pathlib import Path

from PIL import Image
from PIL.ImageQt import ImageQt
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QImage, QPainter, QPalette, QPixmap
from PyQt5.QtPrintSupport import QPrintDialog, QPrinter
from PyQt5.QtWidgets import (
    QAction,
    QApplication,
    QFileDialog,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QMenu,
    QMessageBox,
    QScrollArea,
    QSizePolicy,
    QWidget,
    qApp,
)
from rembg import remove


class QImageViewSync(QWidget):
    def __init__(self, window=None) -> None:
        super().__init__()

        self.window = window
        self.printer = QPrinter()
        self.scaleFactor = 0.0

        self.imageLabelLeft = QLabel()
        self.imageLabelLeft.setBackgroundRole(QPalette.Base)
        self.imageLabelLeft.setSizePolicy(QSizePolicy.Ignored, QSizePolicy.Ignored)
        self.imageLabelLeft.setScaledContents(True)

        self.scrollAreaLeft = QScrollArea()
        self.scrollAreaLeft.setBackgroundRole(QPalette.Dark)
        self.scrollAreaLeft.setWidget(self.imageLabelLeft)
        self.scrollAreaLeft.setVisible(False)

        self.imageLabelRight = QLabel()
        self.imageLabelRight.setBackgroundRole(QPalette.Base)
        self.imageLabelRight.setSizePolicy(QSizePolicy.Ignored, QSizePolicy.Ignored)
        self.imageLabelRight.setScaledContents(True)

        self.scrollAreaRight = QScrollArea()
        self.scrollAreaRight.setBackgroundRole(QPalette.Dark)
        self.scrollAreaRight.setWidget(self.imageLabelRight)
        self.scrollAreaRight.setVisible(False)

        self.centralWidget = QWidget()
        self.layout = QHBoxLayout(self.centralWidget)
        self.layout.addWidget(self.scrollAreaLeft)
        self.layout.addWidget(self.scrollAreaRight)

        self.scrollAreaLeft.verticalScrollBar().valueChanged.connect(
            self.scrollAreaRight.verticalScrollBar().setValue
        )
        self.scrollAreaLeft.horizontalScrollBar().valueChanged.connect(
            self.scrollAreaRight.horizontalScrollBar().setValue
        )
        self.scrollAreaRight.verticalScrollBar().valueChanged.connect(
            self.scrollAreaLeft.verticalScrollBar().setValue
        )
        self.scrollAreaRight.horizontalScrollBar().valueChanged.connect(
            self.scrollAreaLeft.horizontalScrollBar().setValue
        )

        self.scrollAreaLeft.mouseMoveEvent = self.mouse_move_event_left
        self.scrollAreaLeft.mousePressEvent = self.mouse_press_event_left
        self.scrollAreaLeft.mouseReleaseEvent = self.mouse_release_event_left

        self.scrollAreaRight.mouseMoveEvent = self.mouse_move_event_right
        self.scrollAreaRight.mousePressEvent = self.mouse_press_event_right
        self.scrollAreaRight.mouseReleaseEvent = self.mouse_release_event_right

        self.imageLabelLeft.setCursor(Qt.OpenHandCursor)
        self.imageLabelRight.setCursor(Qt.OpenHandCursor)

    def mouse_press_event_left(self, event) -> None:
        self.pressed = True
        self.imageLabelLeft.setCursor(Qt.ClosedHandCursor)
        self.initialPosX = (
            self.scrollAreaLeft.horizontalScrollBar().value() + event.pos().x()
        )
        self.initialPosY = (
            self.scrollAreaLeft.verticalScrollBar().value() + event.pos().y()
        )

    def mouse_release_event_left(self) -> None:
        self.pressed = False
        self.imageLabelLeft.setCursor(Qt.OpenHandCursor)
        self.initialPosX = self.scrollAreaLeft.horizontalScrollBar().value()
        self.initialPosY = self.scrollAreaLeft.verticalScrollBar().value()

    def mouse_move_event_left(self, event) -> None:
        if self.pressed:
            self.scrollAreaLeft.horizontalScrollBar().setValue(
                self.initialPosX - event.pos().x()
            )
            self.scrollAreaLeft.verticalScrollBar().setValue(
                self.initialPosY - event.pos().y()
            )

    def mouse_press_event_right(self, event) -> None:
        self.pressed = True
        self.imageLabelRight.setCursor(Qt.ClosedHandCursor)
        self.initialPosX = (
            self.scrollAreaRight.horizontalScrollBar().value() + event.pos().x()
        )
        self.initialPosY = (
            self.scrollAreaRight.verticalScrollBar().value() + event.pos().y()
        )

    def mouse_release_event_right(self) -> None:
        self.pressed = False
        self.imageLabelRight.setCursor(Qt.OpenHandCursor)
        self.initialPosX = self.scrollAreaRight.horizontalScrollBar().value()
        self.initialPosY = self.scrollAreaRight.verticalScrollBar().value()

    def mouse_move_event_right(self, event) -> None:
        if self.pressed:
            self.scrollAreaRight.horizontalScrollBar().setValue(
                self.initialPosX - event.pos().x()
            )
            self.scrollAreaRight.verticalScrollBar().setValue(
                self.initialPosY - event.pos().y()
            )

    def open(self) -> None:
        options = QFileDialog.Options()
        # file_name = QFileDialog.getOpenFileName(self, "Open File", QDir.currentPath())
        file_name, _ = QFileDialog.getOpenFileName(
            self,
            "QFileDialog.getOpenFileName()",
            "",
            "Images (*.png *.jpeg *.jpg *.bmp *.gif)",
            options=options,
        )
        if file_name:
            image = QImage(file_name)
            if image.isNull():
                QMessageBox.information(
                    self, "Image Viewer", "Cannot load %s." % file_name
                )
                return

            self.imageLabelLeft.setPixmap(QPixmap.fromImage(image))
            self.imageLabelRight.setPixmap(QPixmap.fromImage(image))
            self.scaleFactor = 1.0

            self.scrollAreaLeft.setVisible(True)
            self.scrollAreaRight.setVisible(True)
            self.window.printLeftAct.setEnabled(True)
            self.window.printRightAct.setEnabled(True)
            self.window.fitToWindowAct.setEnabled(True)
            self.update_actions()

            if not self.window.fitToWindowAct.isChecked():
                self.imageLabelLeft.adjustSize()
                self.imageLabelRight.adjustSize()

    def open_left(self) -> None:
        options = QFileDialog.Options()
        self.fileName, _ = QFileDialog.getOpenFileName(
            self,
            "QFileDialog.getOpenFileName()",
            "",
            "Images (*.png *.jpeg *.jpg *.bmp *.gif)",
            options=options,
        )
        if self.fileName:
            image = QImage(self.fileName)
            if image.isNull():
                QMessageBox.information(
                    self, "Image Viewer", "Cannot load %s." % self.fileName
                )
                return

            self.imageLabelLeft.setPixmap(QPixmap.fromImage(image))
            self.scaleFactor = 1.0

            self.scrollAreaLeft.setVisible(True)
            self.window.printLeftAct.setEnabled(True)
            self.window.fitToWindowAct.setEnabled(True)
            self.update_actions()

            if not self.window.fitToWindowAct.isChecked():
                self.imageLabelLeft.adjustSize()

            # Process image on the right using the image from the left
            self.right_image = Image.open(self.fileName)
            self.right_image = remove(self.right_image)
            self.right_image = ImageQt(self.right_image)

            self.imageLabelRight.setPixmap(QPixmap.fromImage(self.right_image))

            self.scrollAreaRight.setVisible(True)
            self.window.printRightAct.setEnabled(True)
            self.window.fitToWindowAct.setEnabled(True)
            self.update_actions()

            if not self.window.fitToWindowAct.isChecked():
                self.imageLabelRight.adjustSize()

    def print_left(self) -> None:
        dialog = QPrintDialog(self.printer, self)
        if dialog.exec_():
            painter = QPainter(self.printer)
            rect = painter.viewport()
            size = self.imageLabelLeft.pixmap().size()
            size.scale(rect.size(), Qt.KeepAspectRatio)
            painter.setViewport(rect.x(), rect.y(), size.width(), size.height())
            painter.setWindow(self.imageLabelLeft.pixmap().rect())
            painter.drawPixmap(0, 0, self.imageLabelLeft.pixmap())

    def print_right(self) -> None:
        dialog = QPrintDialog(self.printer, self)
        if dialog.exec_():
            painter = QPainter(self.printer)
            rect = painter.viewport()
            size = self.imageLabelRight.pixmap().size()
            size.scale(rect.size(), Qt.KeepAspectRatio)
            painter.setViewport(rect.x(), rect.y(), size.width(), size.height())
            painter.setWindow(self.imageLabelRight.pixmap().rect())
            painter.drawPixmap(0, 0, self.imageLabelRight.pixmap())

    def save_right(self) -> None:
        path = Path(self.fileName)
        filename = str(path.stem) + "_no_bg.png"
        self.right_image.save(str(path.parent / filename), "png")
        QMessageBox.information(self, "Image Viewer", "Image saved - %s." % filename)

    def zoom_in(self) -> None:
        self.scale_image(1.25)

    def zoom_out(self) -> None:
        self.scale_image(0.8)

    def normal_size(self) -> None:
        self.imageLabelLeft.adjustSize()
        self.imageLabelRight.adjustSize()
        self.scaleFactor = 1.0

    def about(self) -> None:
        QMessageBox.about(
            self,
            "Image View in the Main Window",
            "<p>The <b>Image Viewer</b> example shows how to combine "
            "QLabel and QScrollArea to display an image. QLabel is "
            "typically used for displaying text, but it can also display "
            "an image. QScrollArea provides a scrolling view around "
            "another widget. If the child widget exceeds the size of the "
            "frame, QScrollArea automatically provides scroll bars.</p>"
            "<p>The example demonstrates how QLabel's ability to scale "
            "its contents (QLabel.scaledContents), and QScrollArea's "
            "ability to automatically resize its contents "
            "(QScrollArea.widgetResizable), can be used to implement "
            "zooming and scaling features.</p>"
            "<p>In addition the example shows how to use QPainter to "
            "print an image.</p>",
        )

    def update_actions(self) -> None:
        self.window.zoomInAct.setEnabled(not self.window.fitToWindowAct.isChecked())
        self.window.zoomOutAct.setEnabled(not self.window.fitToWindowAct.isChecked())
        self.window.normalSizeAct.setEnabled(not self.window.fitToWindowAct.isChecked())

    def scale_image(self, factor) -> None:
        self.scaleFactor *= factor
        self.imageLabelLeft.resize(
            self.scaleFactor * self.imageLabelLeft.pixmap().size()
        )
        self.imageLabelRight.resize(
            self.scaleFactor * self.imageLabelRight.pixmap().size()
        )

        self.adjust_scroll_bar(self.scrollAreaLeft.horizontalScrollBar(), factor)
        self.adjust_scroll_bar(self.scrollAreaLeft.verticalScrollBar(), factor)
        self.adjust_scroll_bar(self.scrollAreaRight.horizontalScrollBar(), factor)
        self.adjust_scroll_bar(self.scrollAreaRight.verticalScrollBar(), factor)

        self.window.zoomInAct.setEnabled(self.scaleFactor < 3.0)
        self.window.zoomOutAct.setEnabled(self.scaleFactor > 0.333)

    @staticmethod
    def adjust_scroll_bar(scroll_bar, factor) -> None:
        scroll_bar.setValue(
            int(
                factor * scroll_bar.value() + ((factor - 1) * scroll_bar.pageStep() / 2)
            )
        )


class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()

        self.imageViewSync = QImageViewSync(window=self)
        self.setCentralWidget(self.imageViewSync.centralWidget)

        # Initialize actions
        self.openLeftAct = QAction(
            "&Open Image",
            self,
            shortcut="Ctrl+O",
            triggered=self.imageViewSync.open_left,
        )

        self.saveRightAct = QAction(
            "&Save Image",
            self,
            shortcut="Ctrl+S",
            triggered=self.imageViewSync.save_right,
        )

        self.printLeftAct = QAction(
            "&Print Left",
            self,
            shortcut="Ctrl+P",
            enabled=False,
            triggered=self.imageViewSync.print_left,
        )
        self.printRightAct = QAction(
            "&Print Right",
            self,
            shortcut="Shift+Ctrl+P",
            enabled=False,
            triggered=self.imageViewSync.print_right,
        )
        self.zoomInAct = QAction(
            "Zoom &In (25%)",
            self,
            shortcut="Ctrl++",
            enabled=False,
            triggered=self.imageViewSync.zoom_in,
        )
        self.zoomOutAct = QAction(
            "Zoom &Out (25%)",
            self,
            shortcut="Ctrl+-",
            enabled=False,
            triggered=self.imageViewSync.zoom_out,
        )
        self.normalSizeAct = QAction(
            "&Normal Size",
            self,
            shortcut="Ctrl+S",
            enabled=False,
            triggered=self.imageViewSync.normal_size,
        )
        self.fitToWindowAct = QAction(
            "&Fit to Window",
            self,
            enabled=False,
            checkable=True,
            shortcut="Ctrl+F",
            triggered=self.fit_to_window,
        )
        self.aboutAct = QAction("&About", self, triggered=self.imageViewSync.about)
        self.aboutQtAct = QAction("About &Qt", self, triggered=qApp.aboutQt)

        # Initialize menus
        self.fileMenu = QMenu("&File", self)
        self.fileMenu.addAction(self.openLeftAct)
        self.fileMenu.addAction(self.saveRightAct)
        self.fileMenu.addSeparator()
        self.fileMenu.addAction(self.printLeftAct)
        self.fileMenu.addAction(self.printRightAct)
        self.fileMenu.addSeparator()

        self.viewMenu = QMenu("&View", self)
        self.viewMenu.addAction(self.zoomInAct)
        self.viewMenu.addAction(self.zoomOutAct)
        self.viewMenu.addAction(self.normalSizeAct)
        self.viewMenu.addSeparator()
        self.viewMenu.addAction(self.fitToWindowAct)

        self.helpMenu = QMenu("&Help", self)
        self.helpMenu.addAction(self.aboutAct)
        self.helpMenu.addAction(self.aboutQtAct)

        self.menuBar().addMenu(self.fileMenu)
        self.menuBar().addMenu(self.viewMenu)
        self.menuBar().addMenu(self.helpMenu)

        self.setWindowTitle("Image View Sync in the Main Window")
        self.resize(1200, 600)

        self.imageViewSync.open_left()

    def fit_to_window(self) -> None:
        fit_to_window = self.fitToWindowAct.isChecked()
        self.imageViewSync.scrollAreaLeft.setWidgetResizable(fit_to_window)
        self.imageViewSync.scrollAreaRight.setWidgetResizable(fit_to_window)
        if not fit_to_window:
            self.imageViewSync.normal_size()

        self.imageViewSync.update_actions()


def main() -> None:
    app = QApplication(sys.argv)
    win = MainWindow()
    win.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
