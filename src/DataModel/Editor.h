#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "Signal.h"
#include "Character.h"
#include "Physics.h"
#include "Workspace.h"

enum class Tool { Select = 0, Move = 1, Resize = 2 };
enum class RunState { Stopped = 0, Running = 1, Paused = 2 };
enum class Lighting { Day = 0, Sunset = 1, Night = 2 };

// The state of the Studio session: the place, the selection, the active tool, undo history.
// Free of Qt and G3D so both the UI and the renderer can share it.
class Editor {
public:
    Editor();

    // ---- place ----
    std::shared_ptr<Workspace> workspace() const { return m_workspace; }
    void newPlace();                                                       // classic baseplate template
    void resetPlace(const std::shared_ptr<Workspace>& ws, const std::string& filePath);
    const std::string& filePath() const { return m_filePath; }
    void setFilePath(const std::string& p);
    bool modified() const { return m_modified; }
    void setModified(bool m);

    // ---- selection / tools ----
    Part* selected() const { return m_selected; }
    void select(Part* p);
    Tool tool() const { return m_tool; }
    void setTool(Tool t);
    float snap() const { return m_snap; }             // 0 = off
    void setSnap(float s);

    // ---- undoable edits ----
    Part* insertPart(const std::string& name, const Vec3& size, int brickColor, PartShape shape = PartShape::Block);   // in front of the camera
    void toggleAnchorSelected();
    void toggleLockSelected();
    void colorSelected(int brickColor);
    void deleteSelected();
    void duplicateSelected();
    void copySelected();
    void cutSelected();
    void paste();
    bool hasClipboard() const { return m_hasClipboard; }
    void setPartState(Part* part, const PartState& state, const std::string& description);
    void rotateSelected(int axis);                    // 90 degrees about world axis 0=X,1=Y,2=Z

    // Interactive edits (mouse drags): apply live, one undo step at the end.
    void beginDrag(Part* part);
    void dragApply(Part* part, const PartState& state);
    void endDrag(Part* part, const std::string& description);

    // ---- history ----
    bool canUndo() const { return !m_undo.empty(); }
    bool canRedo() const { return !m_redo.empty(); }
    std::string undoText() const { return m_undo.empty() ? std::string() : m_undo.back().text; }
    std::string redoText() const { return m_redo.empty() ? std::string() : m_redo.back().text; }
    void undo();
    void redo();

    // ---- camera (reported by the viewport; used to place inserted parts) ----
    void setCamera(const Vec3& position, const Vec3& look) { m_camPos = position; m_camLook = look; }

    // ---- Run (physics) ----
    RunState runState() const { return m_run; }
    bool simulating() const { return m_run != RunState::Stopped; }   // editing is locked while true
    void play();                       // snapshot the place and start the physics (or resume)
    void pause();
    void stop();                       // restore the place exactly as it was before Run
    void playSolo();                   // Run + the player character with a third-person camera
    bool soloMode() const { return m_solo && m_run != RunState::Stopped; }
    Character* character() { return soloMode() ? &m_character : nullptr; }
    // Player input from the viewport: world-space move direction (horizontal, length <= 1) and jump.
    void setCharacterInput(const Vec3& direction, bool jump) { m_inputDir = direction; m_inputJump = jump; }
    void advance(float seconds);       // called every frame by the renderer; steps at a fixed 1/60 s
    unsigned long simVersion() const { return m_simVersion; }   // changes every physics step

    // ---- lighting (time of day) and camera reset ----
    Lighting lighting() const { return m_lighting; }
    void setLighting(Lighting l);
    void requestCameraReset() { m_cameraReset = true; }
    bool takeCameraReset() { const bool r = m_cameraReset; m_cameraReset = false; return r; }
    int partCount() const;

    // ---- output window ----
    void log(const std::string& line);

    // Incremented whenever anything visible changes (the renderer rebuilds its mesh).
    unsigned long version() const { return m_version; }

    // ---- notifications ----
    Signal<>                    selectionChanged;
    Signal<>                    treeChanged;          // parts added / removed / renamed / place replaced
    Signal<>                    toolChanged;
    Signal<>                    snapChanged;
    Signal<>                    lightingChanged;
    Signal<>                    runStateChanged;
    Signal<>                    historyChanged;
    Signal<>                    titleChanged;         // file name or modified flag
    Signal<Part*>               partChanged;
    Signal<const std::string&>  output;

private:
    struct Command {
        std::string text;
        std::function<void()> undo;
        std::function<void()> redo;
    };

    void execute(Command c);          // run redo(), push on the undo stack
    void record(Command c);           // already applied; just push
    void touch();                     // version++, modified
    void applyState(const std::shared_ptr<Part>& p, const PartState& s);
    void attachPart(const std::shared_ptr<Part>& p, std::size_t index);
    void removePart(const std::shared_ptr<Part>& p);
    std::shared_ptr<Part> sharedPart(Part* p) const;
    Vec3 snapPoint(const Vec3& v) const;

    std::shared_ptr<Workspace> m_workspace;
    std::string  m_filePath;
    bool         m_modified = false;
    Part*        m_selected = nullptr;
    Tool         m_tool = Tool::Select;
    float        m_snap = 1.0f;
    unsigned long m_version = 1;
    Lighting     m_lighting = Lighting::Day;
    bool         m_cameraReset = false;

    bool editBlocked();                // true (and logs) while the simulation is running
    RunState     m_run = RunState::Stopped;
    Physics      m_physics;
    std::vector<std::pair<std::shared_ptr<Part>, PartState>> m_snapshot;
    bool         m_modifiedBeforeRun = false;
    bool         m_solo = false;
    Character    m_character;
    Vec3         m_inputDir;
    bool         m_inputJump = false;
    Vec3 spawnPoint(float& yaw) const;
    float        m_accumulator = 0;
    unsigned long m_simVersion = 1;

    std::vector<Command> m_undo, m_redo;

    PartState    m_clipboard;
    bool         m_hasClipboard = false;

    PartState    m_dragStart;
    bool         m_dragging = false;

    Vec3 m_camPos = Vec3(0, 8, 14);
    Vec3 m_camLook = Vec3(0, -0.4f, -1);
};
