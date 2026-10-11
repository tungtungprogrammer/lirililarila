#include "Editor.h"
#include <cmath>

Editor::Editor() {
    newPlace();
}

// ---------------------------------------------------------------- place

void Editor::newPlace() {
    auto ws = std::make_shared<Workspace>();

    PartState base;
    base.name = "Baseplate";
    base.position = Vec3(0, -0.5f, 0);
    base.size = Vec3(512, 1, 512);
    base.brickColor = kBrickBrightGreen;
    ws->createPart(base);

    PartState b;
    b.name = "Part";
    b.brickColor = kBrickBrightRed;    b.position = Vec3(0, 0.6f, 0);    ws->createPart(b);
    b.brickColor = kBrickBrightBlue;   b.position = Vec3(6, 0.6f, 3);    ws->createPart(b);
    b.brickColor = kBrickBrightYellow; b.position = Vec3(-6, 0.6f, -3);  ws->createPart(b);

    resetPlace(ws, std::string());
}

void Editor::resetPlace(const std::shared_ptr<Workspace>& ws, const std::string& filePath) {
    m_workspace = ws;
    m_selected = nullptr;
    m_undo.clear();
    m_redo.clear();
    m_dragging = false;
    m_filePath = filePath;
    m_modified = false;
    m_lighting = Lighting::Day;
    ++m_version;
    treeChanged();
    selectionChanged();
    historyChanged();
    titleChanged();
    lightingChanged();
}

void Editor::setFilePath(const std::string& p) { m_filePath = p; titleChanged(); }

void Editor::setModified(bool m) {
    if (m_modified != m) { m_modified = m; titleChanged(); }
}

void Editor::touch() {
    ++m_version;
    setModified(true);
}

// ---------------------------------------------------------------- selection / tools

void Editor::select(Part* p) {
    if (p == m_selected) { return; }
    m_selected = p;
    selectionChanged();
}

void Editor::setTool(Tool t) {
    if (t == m_tool) { return; }
    m_tool = t;
    toolChanged();
}

void Editor::setSnap(float s) {
    if (s == m_snap) { return; }
    m_snap = s;
    snapChanged();
}

// ---------------------------------------------------------------- helpers

std::shared_ptr<Part> Editor::sharedPart(Part* p) const {
    return p ? std::static_pointer_cast<Part>(p->shared_from_this()) : nullptr;
}

Vec3 Editor::snapPoint(const Vec3& v) const {
    const float s = (m_snap > 0.0f) ? m_snap : 0.0f;
    if (s <= 0.0f) { return v; }
    return Vec3(std::floor(v.x / s + 0.5f) * s, v.y, std::floor(v.z / s + 0.5f) * s);
}

void Editor::applyState(const std::shared_ptr<Part>& p, const PartState& s) {
    const bool renamed = (p->name != s.name);
    p->setState(s);
    touch();
    if (renamed) { treeChanged(); }
    partChanged(p.get());
}

void Editor::attachPart(const std::shared_ptr<Part>& p, std::size_t index) {
    m_workspace->insertChild(index, p);
    touch();
    treeChanged();
}

void Editor::removePart(const std::shared_ptr<Part>& p) {
    if (m_selected == p.get()) { m_selected = nullptr; selectionChanged(); }
    p->detach();
    touch();
    treeChanged();
}

void Editor::execute(Command c) {
    c.redo();
    record(std::move(c));
}

void Editor::record(Command c) {
    m_undo.push_back(std::move(c));
    m_redo.clear();
    touch();
    historyChanged();
}

// ---------------------------------------------------------------- edits

Part* Editor::insertPart(const std::string& name, const Vec3& size, int brickColor, PartShape shape) {
    // Aim at the ground in front of the camera, falling back to a point in the air.
    Vec3 pos = m_camPos + m_camLook * 14.0f;
    if (m_camLook.y < -0.05f) {
        const float t = (0.0f - m_camPos.y) / m_camLook.y;
        if (t > 1.0f && t < 200.0f) { pos = m_camPos + m_camLook * t; }
    }
    pos = snapPoint(pos);
    pos.y = std::max(pos.y, 0.0f) + size.y * 0.5f;

    PartState s;
    s.name = name;
    s.position = pos;
    s.size = size;
    s.brickColor = brickColor;
    s.shape = shape;

    auto part = std::make_shared<Part>();
    part->setState(s);
    const std::size_t index = m_workspace->children().size();

    Command c;
    c.text = "Insert " + name;
    c.redo = [this, part, index]() { attachPart(part, index); select(part.get()); };
    c.undo = [this, part]() { removePart(part); };
    execute(std::move(c));
    log("Inserted " + name);
    return part.get();
}

void Editor::deleteSelected() {
    if (!m_selected) { return; }
    std::shared_ptr<Part> part = sharedPart(m_selected);
    const std::size_t index = m_workspace->indexOf(part.get());
    if (index >= m_workspace->children().size()) { return; }

    Command c;
    c.text = "Delete " + part->name;
    c.redo = [this, part]() { removePart(part); };
    c.undo = [this, part, index]() { attachPart(part, index); select(part.get()); };
    execute(std::move(c));
}

void Editor::copySelected() {
    if (!m_selected) { return; }
    m_clipboard = m_selected->state();
    m_hasClipboard = true;
    log("Copied " + m_clipboard.name);
}

void Editor::cutSelected() {
    if (!m_selected) { return; }
    copySelected();
    deleteSelected();
}

void Editor::paste() {
    if (!m_hasClipboard) { return; }
    PartState s = m_clipboard;
    s.position = s.position + Vec3(1, 0, 1);
    auto part = std::make_shared<Part>();
    part->setState(s);
    const std::size_t index = m_workspace->children().size();

    Command c;
    c.text = "Paste " + s.name;
    c.redo = [this, part, index]() { attachPart(part, index); select(part.get()); };
    c.undo = [this, part]() { removePart(part); };
    execute(std::move(c));
}

void Editor::duplicateSelected() {
    if (!m_selected) { return; }
    PartState s = m_selected->state();
    s.position = s.position + Vec3(1, 0, 1);
    auto part = std::make_shared<Part>();
    part->setState(s);
    const std::size_t index = m_workspace->children().size();

    Command c;
    c.text = "Duplicate " + s.name;
    c.redo = [this, part, index]() { attachPart(part, index); select(part.get()); };
    c.undo = [this, part]() { removePart(part); };
    execute(std::move(c));
}

void Editor::setPartState(Part* part, const PartState& state, const std::string& description) {
    if (!part) { return; }
    const PartState old = part->state();
    if (old == state) { return; }
    std::shared_ptr<Part> sp = sharedPart(part);

    Command c;
    c.text = description;
    c.redo = [this, sp, state]() { applyState(sp, state); };
    c.undo = [this, sp, old]() { applyState(sp, old); };
    execute(std::move(c));
}

void Editor::rotateSelected(int axis) {
    if (!m_selected) { return; }
    PartState s = m_selected->state();
    const Mat3 r = (axis == 0) ? Mat3::rotationX(90) : (axis == 1) ? Mat3::rotationY(90) : Mat3::rotationZ(90);
    s.orientation = r * s.orientation;
    setPartState(m_selected, s, "Rotate " + s.name);
}

void Editor::beginDrag(Part* part) {
    if (!part) { return; }
    m_dragStart = part->state();
    m_dragging = true;
}

void Editor::dragApply(Part* part, const PartState& state) {
    if (!part) { return; }
    part->setState(state);
    ++m_version;
    partChanged(part);
}

void Editor::endDrag(Part* part, const std::string& description) {
    if (!m_dragging || !part) { m_dragging = false; return; }
    m_dragging = false;
    const PartState now = part->state();
    if (now == m_dragStart) { return; }
    std::shared_ptr<Part> sp = sharedPart(part);
    const PartState before = m_dragStart;

    Command c;
    c.text = description;
    c.redo = [this, sp, now]() { applyState(sp, now); };
    c.undo = [this, sp, before]() { applyState(sp, before); };
    record(std::move(c));
}

// ---------------------------------------------------------------- history

void Editor::undo() {
    if (m_undo.empty()) { return; }
    Command c = std::move(m_undo.back());
    m_undo.pop_back();
    c.undo();
    m_redo.push_back(std::move(c));
    ++m_version;
    setModified(true);
    historyChanged();
}

void Editor::redo() {
    if (m_redo.empty()) { return; }
    Command c = std::move(m_redo.back());
    m_redo.pop_back();
    c.redo();
    m_undo.push_back(std::move(c));
    ++m_version;
    setModified(true);
    historyChanged();
}

void Editor::log(const std::string& line) { output(line); }

void Editor::toggleAnchorSelected() {
    if (!m_selected) return;
    PartState s = m_selected->state();
    s.anchored = !s.anchored;
    setPartState(m_selected, s, std::string(s.anchored ? "Anchor " : "Unanchor ") + s.name);
}

void Editor::toggleLockSelected() {
    if (!m_selected) return;
    PartState s = m_selected->state();
    s.locked = !s.locked;
    setPartState(m_selected, s, std::string(s.locked ? "Lock " : "Unlock ") + s.name);
}

void Editor::colorSelected(int brickColor) {
    if (!m_selected) return;
    PartState s = m_selected->state();
    s.brickColor = brickColor;
    setPartState(m_selected, s, "Recolor " + s.name);
}

void Editor::setLighting(Lighting l) {
    if (m_lighting == l) return;
    m_lighting = l;
    touch();
    lightingChanged();
}

int Editor::partCount() const {
    std::vector<Part*> parts;
    m_workspace->collectParts(parts);
    return int(parts.size());
}
