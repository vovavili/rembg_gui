#include <QApplication>

namespace rembg::native_app {

int runMainWindowSmokeTest(int argc, char** argv);

}  // namespace rembg::native_app

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    return rembg::native_app::runMainWindowSmokeTest(argc, argv);
}
