// OldBlox Studio: Qt user interface + G3D viewport in one process.
#include <QApplication>
#include <QCoreApplication>
#include <QTimer>
#include <memory>
#include <string>

#include "DataModel/Editor.h"
#include "Render/IViewport.h"
#include "UI/MainWindow.h"
#include "UI/Theme.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("OldBlox Studio");
    applyClassicTheme(app);

    bool floating = false;
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "--floating-viewport") floating = true;

    Editor editor;
    editor.newPlace();

    MainWindow window(editor);
    window.show();
    QApplication::processEvents();

    // The caption doubles as the key used to find the renderer's native window.
    const std::string caption = "OldBloxViewport_" + std::to_string(QCoreApplication::applicationPid());
    std::unique_ptr<IViewport> viewport(createViewport(editor, caption.c_str(), 900, 600));
    editor.log("Viewport created");

    // Re-parent the G3D window into the Qt viewport (retry for a moment while the window appears).
    QTimer attach;
    int attempts = 0;
    if (!floating) {
        QObject::connect(&attach, &QTimer::timeout, [&] {
            void* handle = viewport->nativeHandle();
            if (handle && window.viewport()->attachNativeWindow(handle)) {
                attach.stop();
                editor.log("Viewport docked");
            } else if (++attempts > 100) {
                attach.stop();
                editor.log("Could not dock the viewport; it stays a separate window (--floating-viewport).");
            }
        });
        attach.start(50);
    }

    QTimer frame;
    QObject::connect(&frame, &QTimer::timeout, [&] {
        if (!viewport->frame()) QApplication::quit();
    });
    frame.setTimerType(Qt::PreciseTimer);
    frame.start(15);

    QObject::connect(&app, &QApplication::aboutToQuit, [&] { frame.stop(); viewport->shutdown(); });
    const int rc = app.exec();
    viewport.reset();
    return rc;
}
