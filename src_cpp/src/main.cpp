#include "MainWindow.h"

#include <QApplication>
#include <QCoreApplication>

using rembg::native_app::MainWindow;

int main(int argc, char* argv[]) {
    QCoreApplication::setOrganizationName("vovavili");
    QCoreApplication::setApplicationName("rembg-gui");

    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return QApplication::exec();
}
