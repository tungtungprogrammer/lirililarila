#include "Studio.h"

G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    GApp::Settings settings(argc, argv);
    settings.window.caption = "OldBlox Studio";
    settings.window.width   = 1280;
    settings.window.height  = 720;
    return Studio(settings).run();
}
