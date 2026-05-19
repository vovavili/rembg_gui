#include <QCoreApplication>

namespace rembg::native_app {

int runImageProcessingTests(int argc, char** argv);
int runIntegrationTests(int argc, char** argv);

}  // namespace rembg::native_app

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    int status = 0;
    status |= rembg::native_app::runImageProcessingTests(argc, argv);
    status |= rembg::native_app::runIntegrationTests(argc, argv);
    return status;
}
