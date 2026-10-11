#pragma once
#include "DataModel/Editor.h"
#include "Geom.h"
#include "MeshBuilder.h"

// Mouse interaction with the 3D scene: picking, face handles (Move / Resize), body dragging.
// Pure C++ (no G3D) so it can be unit-tested.
class Interaction {
public:
    explicit Interaction(Editor& editor) : m_editor(editor) {}

    void mouseDown(const geom::Ray& ray, const Vec3& cameraPos);
    void mouseMove(const geom::Ray& ray, const Vec3& cameraPos, bool leftDown);
    void mouseUp();
    void hover(const geom::Ray& ray, const Vec3& cameraPos);

    bool dragging() const { return m_mode != Mode::Idle; }

    // Selection outline and handles for the current selection. Either mesh may stay empty.
    void buildOverlays(MeshData& selectionBox, MeshData& handles, const Vec3& cameraPos) const;

    Part* pick(const geom::Ray& ray) const;

private:
    enum class Mode { Idle, Handle, Body };
    struct HandleRef { int axis = -1; int sign = 0; };

    float handleScale(const Part& p, const Vec3& cameraPos) const;
    Vec3  handleAnchor(const Part& p, int axis, int sign, float scale) const;
    HandleRef pickHandle(const geom::Ray& ray, const Vec3& cameraPos) const;
    void applyHandleDrag(const geom::Ray& ray);

    Editor&   m_editor;
    Mode      m_mode = Mode::Idle;
    Part*     m_part = nullptr;
    PartState m_start;
    HandleRef m_handle;
    HandleRef m_hover;
    Tool      m_dragTool = Tool::Select;
    Vec3      m_axisDir, m_axisOrigin;
    float     m_t0 = 0;
    Vec3      m_bodyHit0;
};
