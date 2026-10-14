// The G3D side: window, shaders, drawing and input. Everything that can be tested without a GPU
// lives in FlyCamera / MeshBuilder / Interaction.
#include <G3D/G3DAll.h>
using namespace G3D;
#include <memory>
#include <string>

#include "DataModel/Editor.h"
#include "FlyCamera.h"
#include "IViewport.h"
#include "Hud.h"
#include "Interaction.h"
#include "MeshBuilder.h"
#include "Shaders.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

struct GpuMesh {
    AttributeArray pos, nrm, col, aux;
    IndexStream    index;
    int            indexCount = 0;
};

shared_ptr<GpuMesh> upload(const MeshData& m, VertexBuffer::UsageHint hint) {
    if (m.idx.empty()) return shared_ptr<GpuMesh>();
    auto g = std::make_shared<GpuMesh>();
    const int n = m.vertexCount();
    const size_t bytes = size_t(n) * 3 * sizeof(float);
    g->pos = AttributeArray(reinterpret_cast<const Vector3*>(m.pos.data()), n, VertexBuffer::create(bytes, hint));
    g->nrm = AttributeArray(reinterpret_cast<const Vector3*>(m.nrm.data()), n, VertexBuffer::create(bytes, hint));
    g->col = AttributeArray(reinterpret_cast<const Vector3*>(m.col.data()), n, VertexBuffer::create(bytes, hint));
    g->aux = AttributeArray(reinterpret_cast<const Vector3*>(m.aux.data()), n, VertexBuffer::create(bytes, hint));
    g->indexCount = int(m.idx.size());
    g->index = IndexStream(m.idx.data(), g->indexCount, VertexBuffer::create(m.idx.size() * sizeof(int), hint));
    return g;
}

struct LightingPreset {
    float top[3], horizon[3], sun[3], sunColor[3], tint[3];
    float ambient, sunPower, stars, cloud;
};

LightingPreset lightingPreset(Lighting l) {
    switch (l) {
    case Lighting::Sunset:
        return {{0.22f, 0.28f, 0.58f}, {1.00f, 0.62f, 0.34f}, {-0.85f, 0.16f, 0.50f}, {1.0f, 0.55f, 0.25f}, {1.0f, 0.86f, 0.76f},
                0.42f, 0.55f, 0.0f, 0.95f};
    case Lighting::Night:
        return {{0.01f, 0.02f, 0.08f}, {0.07f, 0.09f, 0.22f}, {0.30f, 0.60f, -0.50f}, {0.75f, 0.80f, 1.0f}, {0.55f, 0.62f, 0.98f},
                0.34f, 0.30f, 1.0f, 0.25f};
    default:
        return {{0.20f, 0.45f, 0.85f}, {0.72f, 0.86f, 0.98f}, {-0.35f, 0.85f, 0.40f}, {1.0f, 0.95f, 0.80f}, {1.0f, 1.0f, 1.0f},
                0.46f, 0.62f, 0.0f, 1.0f};
    }
}

GKey letter(char c) { return GKey(GKey::Value(c)); }

}  // namespace

class StudioApp : public GApp {
public:
    StudioApp(const GApp::Settings& settings, Editor& editor)
        : GApp(settings), m_editor(editor), m_interaction(editor) {}

    // GApp keeps these protected; the embedding application drives the loop itself.
    void start() { beginRun(); }
    bool step() { oneFrame(); return !m_endProgram; }
    void stop() { endRun(); }

    void onInit() override {
        GApp::onInit();
        showRenderingStats = false;
        // G3D normally swaps buffers from inside its own (HDR + film) draw path, which we bypass.
        // Without this the back buffer is never presented: black / stale / garbage frames.
        setSubmitToDisplayMode(SubmitToDisplayMode::MINIMIZE_LATENCY);
        // The Qt timer paces the frames. G3D must not drop to 4 fps when the Qt side has focus.
        setLowerFrameRateInBackground(false);
        setCameraManipulator(shared_ptr<Manipulator>());       // we do our own camera

        FilmSettings& film = activeCamera()->filmSettings();
        film.setBloomStrength(0.0f);
        film.setVignetteTopStrength(0.0f);
        film.setVignetteBottomStrength(0.0f);
        film.setGamma(2.2f);

        Shader::Specification scene;
        scene[Shader::VERTEX] = Shader::Source(Shader::STRING, shaders::kSceneVertex);
        scene[Shader::PIXEL]  = Shader::Source(Shader::STRING, shaders::kSceneFragment);
        m_sceneShader = Shader::create(scene);

        Shader::Specification sky;
        sky[Shader::VERTEX] = Shader::Source(Shader::STRING, shaders::kSkyVertex);
        sky[Shader::PIXEL]  = Shader::Source(Shader::STRING, shaders::kSkyFragment);
        m_skyShader = Shader::create(sky);

        Array<Vector4> tri;
        tri.append(Vector4(-1, -1, 0, 1), Vector4(3, -1, 0, 1), Vector4(-1, 3, 0, 1));
        m_skyPos = AttributeArray(tri, VertexBuffer::create(tri.size() * sizeof(Vector4), VertexBuffer::WRITE_ONCE));

        m_editor.log("Rendering initialised");
    }

    void onUserInput(UserInput* ui) override {
        GApp::onUserInput(ui);
        const float w = std::max(1, window()->width());
        const float h = std::max(1, window()->height());
        const Vector2 mouse = ui->mouseXY();
        const bool left = ui->keyDown(GKey::LEFT_MOUSE);
        const bool right = ui->keyDown(GKey::RIGHT_MOUSE);
        const float aspect = w / h;
        const float ndcX = mouse.x / w * 2.0f - 1.0f;
        const float ndcY = 1.0f - mouse.y / h * 2.0f;
        const bool inside = mouse.x >= 0 && mouse.y >= 0 && mouse.x < w && mouse.y < h;

        // ---- Play Solo: the player and a third-person camera replace the editor controls ----
        if (Character* ch = m_editor.character()) {
            if (!m_wasSolo) {
                m_savedCamera = m_camera;
                m_orbitYaw = ch->yaw;
                m_orbitPitch = -0.3f;
                m_orbitDist = 14.0f;
                m_wasSolo = true;
                m_soloTime = 0;
            }
            if (right) {
                m_orbitYaw -= (mouse.x - m_lastMouse.x) * 0.005f;
                m_orbitPitch = std::max(-1.3f, std::min(1.3f, m_orbitPitch - (mouse.y - m_lastMouse.y) * 0.005f));
            }
            const Vec3 f(-std::sin(m_orbitYaw), 0, -std::cos(m_orbitYaw));
            const Vec3 r(std::cos(m_orbitYaw), 0, -std::sin(m_orbitYaw));
            Vec3 dir;
            if (ui->keyDown(letter('w')) || ui->keyDown(GKey::UP)) dir = dir + f;
            if (ui->keyDown(letter('s')) || ui->keyDown(GKey::DOWN)) dir = dir - f;
            if (ui->keyDown(letter('d')) || ui->keyDown(GKey::RIGHT)) dir = dir + r;
            if (ui->keyDown(letter('a')) || ui->keyDown(GKey::LEFT)) dir = dir - r;
            m_editor.setCharacterInput(dir, ui->keyDown(GKey::SPACE));
            m_lastMouse = mouse;
            m_leftWas = left;
            return;
        }
        if (m_wasSolo) {                                   // solo ended: back to the editor camera
            m_camera = m_savedCamera;
            m_wasSolo = false;
            m_editor.setCharacterInput(Vec3(), false);
        }

        // ---- camera ----
        if (right) {
            m_camera.look(mouse.x - m_lastMouse.x, mouse.y - m_lastMouse.y);
        }
        const float speed = (ui->keyDown(GKey::LSHIFT) ? 48.0f : 16.0f) * m_dt;
        Vec3 move;
        if (ui->keyDown(letter('w'))) move = move + m_camera.forward();
        if (ui->keyDown(letter('s'))) move = move - m_camera.forward();
        if (ui->keyDown(letter('d'))) move = move + m_camera.right();
        if (ui->keyDown(letter('a'))) move = move - m_camera.right();
        if (ui->keyDown(letter('e'))) move = move + Vec3(0, 1, 0);
        if (ui->keyDown(letter('q'))) move = move - Vec3(0, 1, 0);
        m_camera.position = m_camera.position + move * speed;

        // ---- mouse ----
        const geom::Ray ray = m_camera.ray(ndcX, ndcY, aspect);
        if (left && !m_leftWas && inside && !right) {
            m_interaction.mouseDown(ray, m_camera.position);
        } else if (left && m_leftWas) {
            m_interaction.mouseMove(ray, m_camera.position, true);
        } else if (!left && m_leftWas) {
            m_interaction.mouseUp();
        } else if (inside && !right) {
            m_interaction.hover(ray, m_camera.position);
        }
        m_leftWas = left;
        m_lastMouse = mouse;

        // ---- keyboard shortcuts (the Qt menu handles them when the Qt side has focus) ----
        const bool ctrl = ui->keyDown(GKey::LCTRL) || ui->keyDown(GKey::RCTRL);
        if (ctrl) {
            if (ui->keyPressed(letter('z'))) m_editor.undo();
            if (ui->keyPressed(letter('y'))) m_editor.redo();
            if (ui->keyPressed(letter('c'))) m_editor.copySelected();
            if (ui->keyPressed(letter('x'))) m_editor.cutSelected();
            if (ui->keyPressed(letter('v'))) m_editor.paste();
            if (ui->keyPressed(letter('d'))) m_editor.duplicateSelected();
            if (ui->keyPressed(letter('r'))) m_editor.rotateSelected(1);
            if (ui->keyPressed(letter('t'))) m_editor.rotateSelected(2);
        } else {
            if (ui->keyPressed(GKey::DELETE)) m_editor.deleteSelected();
            if (ui->keyPressed(letter('1'))) m_editor.setTool(Tool::Select);
            if (ui->keyPressed(letter('2'))) m_editor.setTool(Tool::Move);
            if (ui->keyPressed(letter('3'))) m_editor.setTool(Tool::Resize);
            if (ui->keyPressed(GKey::ESCAPE)) m_editor.select(nullptr);
            if (ui->keyPressed(letter('f'))) focusSelection();
        }
    }

    bool onEvent(const GEvent& e) override {
        if (e.type == GEventType::MOUSE_SCROLL_2D && m_editor.soloMode()) {
            m_orbitDist = std::max(0.5f, std::min(80.0f, m_orbitDist * std::pow(0.9f, float(e.scroll2d.dy))));
            return true;
        }
        if (e.type == GEventType::MOUSE_SCROLL_2D) {
            m_camera.position = m_camera.position + m_camera.forward() * (float(e.scroll2d.dy) * 2.0f);
            return true;
        }
        // G3D must see resize events (it reallocates its framebuffer there); everything else of its
        // default handling (F-keys, shader reload, ...) is skipped on purpose.
        if (e.type == GEventType::VIDEO_RESIZE) {
            // Not forwarded to GApp::resize: that reallocates the (unused) HDR buffers on every
            // pixel of a drag-resize, which made resizing crawl. We draw straight to the window.
            m_editor.log("Viewport resized to " + std::to_string(e.resize.w) + "x" + std::to_string(e.resize.h));
        }
        return false;
    }

    // Never sleep inside G3D: it blocks the whole Qt UI. The Qt timer sets the pace.
    void onWait(RealTime) override {}

    void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override {
        GApp::onSimulation(rdt, sdt, idt);
        if (m_editor.takeCameraReset()) m_camera = FlyCamera();
        m_editor.advance(float(rdt));
        if (Character* ch = m_editor.character()) {
            updateSoloCamera(*ch);
            if (m_editor.runState() == RunState::Running) m_soloTime += float(std::min(rdt, 0.1));
        }
        m_dt = float(std::min(rdt, 0.1));
        m_editor.setCamera(m_camera.position, m_camera.forward());
    }

    void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface>>&) override {
        const float w = std::max(1, window()->width());
        const float h = std::max(1, window()->height());
        float vp[16];
        m_camera.viewProjection(w / h, vp);

        // We draw straight into the window (no HDR/film pass), so pin the target state to the real
        // window size every frame; G3D's own viewport can lag behind after the window was re-parented.
        rd->setFramebuffer(window()->framebuffer());
        rd->setSRGBConversion(false);
        rd->setClip2D(Rect2D::inf());
        rd->setViewport(Rect2D::xywh(0.0f, 0.0f, w, h));
        glViewport(0, 0, int(w), int(h));
        glDisable(GL_SCISSOR_TEST);

        if (m_meshVersion != m_editor.version() || m_simVersionSeen != m_editor.simVersion()) rebuildScene();
        const LightingPreset L = lightingPreset(m_editor.lighting());

        rd->setColorClearValue(Color3(L.horizon[0], L.horizon[1], L.horizon[2]));
        rd->clear();

        // sky
        rd->pushState();
        rd->setDepthTest(RenderDevice::DEPTH_ALWAYS_PASS);
        rd->setDepthWrite(false);
        rd->setCullFace(CullFace::NONE);
        {
            Args args;
            args.setPrimitiveType(PrimitiveType::TRIANGLES);
            args.setAttributeArray("aPos", m_skyPos);
            const Vec3 f = m_camera.forward(), r = m_camera.right(), u = m_camera.up();
            args.setUniform("uFwd", Vector3(f.x, f.y, f.z), true);
            args.setUniform("uRight", Vector3(r.x, r.y, r.z), true);
            args.setUniform("uUp", Vector3(u.x, u.y, u.z), true);
            args.setUniform("uTan", Vector2(m_camera.tanHalfFov() * (w / h), m_camera.tanHalfFov()), true);
            args.setUniform("uSkyTop", Vector3(L.top[0], L.top[1], L.top[2]), true);
            args.setUniform("uSkyHorizon", Vector3(L.horizon[0], L.horizon[1], L.horizon[2]), true);
            args.setUniform("uSunDirSky", Vector3(L.sun[0], L.sun[1], L.sun[2]), true);
            args.setUniform("uSunColor", Vector3(L.sunColor[0], L.sunColor[1], L.sunColor[2]), true);
            args.setUniform("uStars", L.stars, true);
            args.setUniform("uCloudLight", L.cloud, true);
            rd->apply(m_skyShader, args);
        }
        rd->popState();

        // parts
        rd->pushState();
        rd->setDepthTest(RenderDevice::DEPTH_LESS);
        rd->setDepthWrite(true);
        rd->setCullFace(CullFace::BACK);
        drawMesh(rd, m_scene, vp, false, L);
        drawMesh(rd, m_sceneDyn, vp, false, L);
        if (Character* ch = m_editor.character()) {
            if (m_camDist > 1.8f) {                                    // hidden in first person
                MeshData body;
                for (Part* p : ch->parts()) meshbuilder::addPart(body, *p);
                for (const Character::FaceBox& fb : ch->face()) meshbuilder::addBox(body, fb.center, fb.rot, fb.half, Vec3(0.04f, 0.04f, 0.04f));
                meshbuilder::addStud(body, ch->headStudBase(), ch->headStudRotation(), meshbuilder::brickRgb(kBrickBrightYellow));
                drawMesh(rd, upload(body, VertexBuffer::WRITE_EVERY_FRAME), vp, false, L);
            }
        }
        rd->popState();

        // see-through parts, blended over everything opaque
        if (m_sceneAlpha) {
            rd->pushState();
            rd->setDepthTest(RenderDevice::DEPTH_LESS);
            rd->setDepthWrite(false);
            rd->setCullFace(CullFace::BACK);
            rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
            drawMesh(rd, m_sceneAlpha, vp, false, L);
            rd->popState();
        }

        // soft blob shadow under the player, so jumps and edges can be judged
        if (Character* ch = m_editor.character()) {
            if (!ch->dead || ch->deadTime < 0.4f) {
                MeshData sh;
                if (shadowDisc(*ch, sh)) {
                    rd->pushState();
                    rd->setDepthTest(RenderDevice::DEPTH_LEQUAL);
                    rd->setDepthWrite(false);
                    rd->setCullFace(CullFace::NONE);
                    rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
                    drawMesh(rd, upload(sh, VertexBuffer::WRITE_EVERY_FRAME), vp, false, L);
                    rd->popState();
                }
            }
        }

        // heads-up display (health bar, hints, death screen), drawn flat over everything
        if (Character* ch = m_editor.character()) {
            HudState hs;
            hs.width = int(w); hs.height = int(h);
            hs.solo = true;
            hs.paused = m_editor.runState() == RunState::Paused;
            hs.health = ch->health; hs.maxHealth = ch->maxHealth;
            hs.dead = ch->dead; hs.deadTime = ch->deadTime;
            hs.soloTime = m_soloTime;
            MeshData hm;
            hud::build(hs, hm);
            if (!hm.idx.empty()) {
                static const float kIdentity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
                rd->pushState();
                rd->setDepthTest(RenderDevice::DEPTH_ALWAYS_PASS);
                rd->setDepthWrite(false);
                rd->setCullFace(CullFace::NONE);
                rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
                drawMesh(rd, upload(hm, VertexBuffer::WRITE_EVERY_FRAME), kIdentity, true, L);
                rd->popState();
            }
        }

        // selection box and handles (rebuilt every frame; they are tiny)
        MeshData box, handles;
        m_interaction.buildOverlays(box, handles, m_camera.position);
        if (!box.idx.empty()) {
            rd->pushState();
            rd->setDepthTest(RenderDevice::DEPTH_LEQUAL);
            rd->setCullFace(CullFace::BACK);
            drawMesh(rd, upload(box, VertexBuffer::WRITE_EVERY_FRAME), vp, true, L);
            rd->popState();
        }
        if (!handles.idx.empty()) {
            rd->pushState();
            rd->setDepthTest(RenderDevice::DEPTH_ALWAYS_PASS);
            rd->setDepthWrite(false);
            rd->setCullFace(CullFace::BACK);
            drawMesh(rd, upload(handles, VertexBuffer::WRITE_EVERY_FRAME), vp, true, L);
            rd->popState();
        }
    }

    // Keep G3D's own 2D overlay (profiler text, GUI) out of the picture.
    void onGraphics2D(RenderDevice*, Array<shared_ptr<Surface2D>>&) override {}

private:
    // Anchored opaque parts never move, so they are only rebuilt when the user edits something.
    // Everything that can move (or is see-through) is rebuilt whenever the physics advances.
    void rebuildScene() {
        std::vector<Part*> parts;
        m_editor.workspace()->collectParts(parts);
        const bool staticStale = m_meshVersion != m_editor.version();
        MeshData fixedMesh, moving, glass;
        for (const Part* p : parts) {
            if (p->transparency >= 0.95f) continue;                    // invisible
            if (p->transparency > 0.0f) meshbuilder::addPart(glass, *p);
            else if (!p->anchored || p->behavior == PartBehavior::Spin) meshbuilder::addPart(moving, *p);
            else if (staticStale)       meshbuilder::addPart(fixedMesh, *p);
        }
        if (staticStale) m_scene = upload(fixedMesh, VertexBuffer::WRITE_ONCE);
        const VertexBuffer::UsageHint hint = m_editor.simulating() ? VertexBuffer::WRITE_EVERY_FRAME : VertexBuffer::WRITE_ONCE;
        m_sceneDyn = upload(moving, hint);
        m_sceneAlpha = upload(glass, hint);
        m_meshVersion = m_editor.version();
        m_simVersionSeen = m_editor.simVersion();
    }

    void drawMesh(RenderDevice* rd, const shared_ptr<GpuMesh>& g, const float vp[16], bool flat, const LightingPreset& L) {
        if (!g) return;
        Args args;
        args.setPrimitiveType(PrimitiveType::TRIANGLES);
        args.setAttributeArray("aPos", g->pos);
        args.setAttributeArray("aNormal", g->nrm);
        args.setAttributeArray("aColor", g->col);
        args.setAttributeArray("aAux", g->aux);
        args.setIndexStream(g->index);
        args.setUniform("vp0", Vector4(vp[0], vp[1], vp[2], vp[3]), true);
        args.setUniform("vp1", Vector4(vp[4], vp[5], vp[6], vp[7]), true);
        args.setUniform("vp2", Vector4(vp[8], vp[9], vp[10], vp[11]), true);
        args.setUniform("vp3", Vector4(vp[12], vp[13], vp[14], vp[15]), true);
        args.setUniform("uFlat", flat ? 1.0f : 0.0f, true);
        args.setUniform("uSunDir", Vector3(L.sun[0], L.sun[1], L.sun[2]), true);
        args.setUniform("uTint", Vector3(L.tint[0], L.tint[1], L.tint[2]), true);
        args.setUniform("uAmbient", L.ambient, true);
        args.setUniform("uSunPower", L.sunPower, true);
        rd->apply(m_sceneShader, args);
    }

    // A dark translucent disc on whatever is below the player's feet. Returns false if there is nothing below.
    bool shadowDisc(const Character& ch, MeshData& out) {
        std::vector<Part*> parts;
        m_editor.workspace()->collectParts(parts);
        const geom::Ray ray{ch.feet + Vec3(0, 0.6f, 0), Vec3(0, -1, 0)};
        float best = 1e30f;
        for (const Part* p : parts) {
            if (p->transparency > 0.0f || !p->canCollide) continue;
            const float t = geom::intersectBox(ray, p->position, p->orientation, p->size * 0.5f);
            if (t >= 0 && t < best) best = t;
        }
        if (best > 60.0f) return false;
        const float k = std::max(0.35f, 1.0f - best / 40.0f);              // fades and shrinks as the player rises
        const Vec3 c = ray.origin + ray.dir * best + Vec3(0, 0.04f, 0);
        const float r = 1.5f * k;
        const Vec3 aux(0, 0, 2.0f * 6.0f);                                  // 60% transparent (see MeshBuilder)
        const int centre = out.addVertex(c, Vec3(0, 1, 0), Vec3(0, 0, 0), aux);
        int ring[20];
        for (int i = 0; i < 20; ++i) {
            const float a = 6.2831853f * i / 20.0f;
            ring[i] = out.addVertex(c + Vec3(std::cos(a) * r, 0, std::sin(a) * r), Vec3(0, 1, 0), Vec3(0, 0, 0), aux);
        }
        for (int i = 0; i < 20; ++i) out.addTri(centre, ring[i], ring[(i + 1) % 20], Vec3(0, 1, 0));
        return true;
    }

    // Orbit camera behind the player; pulled in when something is in the way.
    void updateSoloCamera(const Character& ch) {
        const Vec3 focus = ch.focusPosition();
        m_camera.yaw = m_orbitYaw;
        m_camera.pitch = m_orbitPitch;
        const Vec3 back = m_camera.forward() * -1.0f;
        float dist = m_orbitDist;
        std::vector<Part*> parts;
        m_editor.workspace()->collectParts(parts);
        const geom::Ray ray{focus, back};
        for (const Part* p : parts) {
            if (p->transparency > 0.0f) continue;
            const float t = geom::intersectBox(ray, p->position, p->orientation, p->size * 0.5f);
            if (t >= 0 && t < dist) dist = t - 0.3f;
        }
        m_camDist = std::max(0.3f, dist);
        m_camera.position = focus + back * m_camDist;
    }

    void focusSelection() {
        if (Part* p = m_editor.selected()) {
            const float radius = std::max(2.0f, p->size.length());
            m_camera.position = p->position - m_camera.forward() * (radius * 1.8f);
            m_camera.lookAt(p->position);
        }
    }

    Editor&       m_editor;
    Interaction   m_interaction;
    FlyCamera     m_camera;
    float         m_dt = 0.016f;
    bool          m_leftWas = false;
    Vector2       m_lastMouse;
    unsigned long m_meshVersion = 0;

    shared_ptr<Shader>  m_sceneShader, m_skyShader;
    AttributeArray      m_skyPos;
    shared_ptr<GpuMesh> m_scene, m_sceneDyn, m_sceneAlpha;
    unsigned long m_simVersionSeen = 0;

    bool      m_wasSolo = false;
    float     m_soloTime = 0;
    FlyCamera m_savedCamera;
    float     m_orbitYaw = 0, m_orbitPitch = -0.3f, m_orbitDist = 14.0f, m_camDist = 14.0f;
};

class ViewportImpl : public IViewport {
public:
    ViewportImpl(Editor& editor, const char* caption, int w, int h) : m_editor(editor), m_caption(caption) {
        GApp::Settings settings;
        settings.window.caption = caption;
        settings.window.width = w;
        settings.window.height = h;
        settings.window.resizable = true;
        settings.useDeveloperTools = false;
        settings.writeLicenseFile = false;
        m_app.reset(new StudioApp(settings, editor));
        m_app->start();
    }

    bool frame() override {
        try { return m_app->step(); }
        catch (const std::exception& e) { m_editor.log(std::string("Renderer error: ") + e.what()); }
        catch (...) { m_editor.log("Renderer error (unknown)"); }
        return true;
    }

    void* nativeHandle() override {
#ifdef _WIN32
        return ::FindWindowA(nullptr, m_caption.c_str());
#else
        return nullptr;
#endif
    }

    void shutdown() override {
        if (m_app) { m_app->stop(); m_app.reset(); }
    }

    ~ViewportImpl() override { shutdown(); }

private:
    Editor&                    m_editor;
    std::string                m_caption;
    std::unique_ptr<StudioApp> m_app;
};

IViewport* createViewport(Editor& editor, const char* caption, int width, int height) {
    return new ViewportImpl(editor, caption, width, height);
}
