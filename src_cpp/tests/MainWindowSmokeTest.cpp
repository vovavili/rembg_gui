#include "MainWindow.h"

#include <QAction>
#include <QtTest>

namespace rembg::native_app {

class MainWindowSmokeTest : public QObject {
    Q_OBJECT

private slots:
    void constructsWithoutOpeningDialog() {
        MainWindow window(false);
        QCOMPARE(window.windowTitle(), QString("Rembg"));
        QVERIFY(window.saveRightAction() != nullptr);
        QVERIFY(!window.saveRightAction()->isEnabled());
    }
};

int runMainWindowSmokeTest(int argc, char** argv) {
    MainWindowSmokeTest tests;
    return QTest::qExec(&tests, argc, argv);
}

}  // namespace rembg::native_app

#include "MainWindowSmokeTest.moc"
