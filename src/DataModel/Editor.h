#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "Signal.h"
#include "Workspace.h"

enum class Tool { Select = 0, Move = 1, Resize = 2 };

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
    Part* insertPart(const std::string& name, const Vec3& size, int brickColor);   // in front of the camera
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

    // ---- output window ----
    void log(const std::string& line);

    // Incremented whenever anything visible changes (the renderer rebuilds its mesh).
    unsigned long version() const { return m_version; }

    // ---- notifications ----
    Signal<>                    selectionChanged;
    Signal<>                    treeChanged;          // parts added / removed / renamed / place replaced
    Signal<>                    toolChanged;
    Signal<>                    snapChanged;
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

    std::vector<Command> m_undo, m_redo;

    PartState    m_clipboard;
    bool         m_hasClipboard = false;

    PartState    m_dragStart;
    bool         m_dragging = false;

    Vec3 m_camPos = Vec3(0, 8, 14);
    Vec3 m_camLook = Vec3(0, -0.4f, -1);
};
