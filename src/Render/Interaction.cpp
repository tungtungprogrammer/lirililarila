#include "Interaction.h"

namespace {
const float kMinSize = 0.2f;
const Vec3 kHandleColor(0.30f, 0.62f, 1.0f);
const Vec3 kHandleHover(1.0f, 0.95f, 0.35f);
const Vec3 kSelectColor(0.10f, 0.50f, 1.0f);
}

Part* Interaction::pick(const geom::Ray& ray) const {
    std::vector<Part*> parts;
    m_editor.workspace()->collectParts(parts);
    Part* best = nullptr;
    float bestT = 1e30f;
    for (Part* p : parts) {
        if (p->locked) continue;
        const float t = geom::intersectBox(ray, p->position, p->orientation, p->size * 0.5f);
        if (t >= 0 && t < bestT) { bestT = t; best = p; }
    }
    return best;
}

float Interaction::handleScale(const Part& p, const Vec3& cam) const {
    const float dist = (p.position - cam).length();
    return std::max(0.7f, std::min(8.0f, dist * 0.055f));
}

Vec3 Interaction::handleAnchor(const Part& p, int axis, int sign, float scale) const {
    const Vec3 d = geom::column(p.orientation, axis) * float(sign);
    return p.position + d * (geom::comp(p.size, axis) * 0.5f + scale * 1.1f);
}

Interaction::HandleRef Interaction::pickHandle(const geom::Ray& ray, const Vec3& cam) const {
    HandleRef best;
    Part* p = m_editor.selected();
    if (!p || m_editor.tool() == Tool::Select || m_editor.simulating()) return best;
    const float s = handleScale(*p, cam);
    float bestT = 1e30f;
    for (int axis = 0; axis < 3; ++axis)
        for (int sign = -1; sign <= 1; sign += 2) {
            const float t = geom::intersectSphere(ray, handleAnchor(*p, axis, sign, s), s * 0.6f);
            if (t >= 0 && t < bestT) { bestT = t; best.axis = axis; best.sign = sign; }
        }
    return best;
}

void Interaction::hover(const geom::Ray& ray, const Vec3& cam) {
    m_hover = dragging() ? m_handle : pickHandle(ray, cam);
}

void Interaction::mouseDown(const geom::Ray& ray, const Vec3& cam) {
    if (m_editor.simulating()) { m_editor.select(pick(ray)); return; }     // no dragging while Run is active
    const HandleRef h = pickHandle(ray, cam);
    Part* sel = m_editor.selected();
    if (h.axis >= 0 && sel) {
        m_mode = Mode::Handle;
        m_part = sel;
        m_handle = h;
        m_dragTool = m_editor.tool();
        m_start = sel->state();
        m_axisDir = geom::column(sel->orientation, h.axis) * float(h.sign);
        m_axisOrigin = (m_dragTool == Tool::Move)
                           ? sel->position
                           : sel->position + m_axisDir * (geom::comp(sel->size, h.axis) * 0.5f);
        if (!geom::closestOnAxis(m_axisOrigin, m_axisDir, ray, m_t0)) m_t0 = 0;
        m_editor.beginDrag(sel);
        return;
    }

    Part* hit = pick(ray);
    m_editor.select(hit);
    if (hit) {
        // Dragging the body slides it over the horizontal plane through its centre.
        const float denom = ray.dir.y;
        if (std::fabs(denom) > 1e-4f) {
            const float t = (hit->position.y - ray.origin.y) / denom;
            if (t > 0) {
                m_mode = Mode::Body;
                m_part = hit;
                m_start = hit->state();
                m_bodyHit0 = ray.origin + ray.dir * t;
                m_editor.beginDrag(hit);
            }
        }
    }
}

void Interaction::applyHandleDrag(const geom::Ray& ray) {
    float t;
    if (!m_part || !geom::closestOnAxis(m_axisOrigin, m_axisDir, ray, t)) return;
    const float snap = m_editor.snap();
    const float delta = geom::snapTo(t - m_t0, snap);
    PartState s = m_start;
    if (m_dragTool == Tool::Move) {
        s.position = m_start.position + m_axisDir * delta;
    } else {
        const float startLen = geom::comp(m_start.size, m_handle.axis);
        const float newLen = std::max(kMinSize, startLen + delta);
        geom::setComp(s.size, m_handle.axis, newLen);
        s.position = m_start.position + m_axisDir * ((newLen - startLen) * 0.5f);
    }
    m_editor.dragApply(m_part, s);
}

void Interaction::mouseMove(const geom::Ray& ray, const Vec3& cam, bool leftDown) {
    if (!leftDown) { hover(ray, cam); return; }
    if (m_mode == Mode::Handle) {
        applyHandleDrag(ray);
    } else if (m_mode == Mode::Body && m_part && std::fabs(ray.dir.y) > 1e-4f) {
        const float t = (m_start.position.y - ray.origin.y) / ray.dir.y;
        if (t <= 0) return;
        const Vec3 hit = ray.origin + ray.dir * t;
        PartState s = m_start;
        s.position.x += geom::snapTo(hit.x - m_bodyHit0.x, m_editor.snap());
        s.position.z += geom::snapTo(hit.z - m_bodyHit0.z, m_editor.snap());
        m_editor.dragApply(m_part, s);
    }
}

void Interaction::mouseUp() {
    if (m_mode != Mode::Idle && m_part) {
        const char* what = m_mode == Mode::Body ? "Move" : (m_dragTool == Tool::Move ? "Move" : "Resize");
        m_editor.endDrag(m_part, what);
    }
    m_mode = Mode::Idle;
    m_part = nullptr;
    m_handle = HandleRef();
}

void Interaction::buildOverlays(MeshData& box, MeshData& handles, const Vec3& cam) const {
    Part* p = m_editor.selected();
    if (!p) return;
    meshbuilder::addFrame(box, p->position, p->orientation, p->size * 0.5f + Vec3(0.02f, 0.02f, 0.02f), 0.03f, kSelectColor);
    if (m_editor.tool() == Tool::Select || m_editor.simulating()) return;

    const float s = handleScale(*p, cam);
    for (int axis = 0; axis < 3; ++axis)
        for (int sign = -1; sign <= 1; sign += 2) {
            if (dragging() && !(m_handle.axis == axis && m_handle.sign == sign)) continue;
            const bool hot = (m_hover.axis == axis && m_hover.sign == sign);
            const Vec3 d = geom::column(p->orientation, axis) * float(sign);
            const Vec3 faceCentre = p->position + d * (geom::comp(p->size, axis) * 0.5f);
            const Vec3 color = hot ? kHandleHover : kHandleColor;
            if (m_editor.tool() == Tool::Move)
                meshbuilder::addArrow(handles, faceCentre + d * (s * 0.25f), d, s * 1.9f, s * 0.5f, color);
            else {
                // resize handle: a small cube on a stalk, as in the original
                const Vec3 c = handleAnchor(*p, axis, sign, s);
                meshbuilder::addBox(handles, c, p->orientation, Vec3(s * 0.4f, s * 0.4f, s * 0.4f), color);
                meshbuilder::addBox(handles, faceCentre + d * (s * 0.5f), p->orientation, Vec3(0.03f * s, 0.03f * s, 0.03f * s) + Vec3(), color);
            }
        }
}
