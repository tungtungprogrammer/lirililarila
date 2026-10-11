#pragma once
// Interface between the Qt application and the G3D renderer. Free of G3D and Qt types so both sides can include it.
class Editor;

class IViewport {
public:
    virtual ~IViewport() {}
    // Runs one frame (input, simulation, drawing). Returns false when the renderer wants to quit.
    virtual bool frame() = 0;
    // The renderer's native window (an HWND on Windows), or nullptr if it cannot be found.
    virtual void* nativeHandle() = 0;
    virtual void shutdown() = 0;
};

// Creates the renderer. `caption` must be unique: it is how the native window is located for embedding.
IViewport* createViewport(Editor& editor, const char* caption, int width, int height);
