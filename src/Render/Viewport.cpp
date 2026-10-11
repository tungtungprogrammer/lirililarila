// The G3D side: window, shaders, drawing and input. Everything that can be tested without a GPU
// lives in FlyCamera / MeshBuilder / Interaction.
#include <G3D/G3DAll.h>
using namespace G3D;
#include <memory>
#include <string>

#include "DataModel/Editor.h"
#include "FlyCamera.h"
#include "IViewport.h"
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
        if (e.type == GEventType::MOUSE_SCROLL_2D) {
            m_camera.position = m_camera.position + m_camera.forward() * (float(e.scroll2d.dy) * 2.0f);
            return true;
        }
        // G3D must see resize events (it reallocates its framebuffer there); everything else of its
        // default handling (F-keys, shader reload, ...) is skipped on purpose.
        if (e.type == GEventType::VIDEO_RESIZE) {
            GApp::onEvent(e);
            m_editor.log("Viewport resized to " + std::to_string(e.resize.w) + "x" + std::to_string(e.resize.h));
        }
        return false;
    }

    void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override {
        GApp::onSimulation(rdt, sdt, idt);
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
        rd->setFramebuffer(shared_ptr<Framebuffer>());
        rd->setSRGBConversion(false);
        rd->setClip2D(Rect2D::inf());
        rd->setViewport(Rect2D::xywh(0.0f, 0.0f, w, h));
        glViewport(0, 0, int(w), int(h));
        glDisable(GL_SCISSOR_TEST);

        if (m_meshVersion != m_editor.version()) rebuildScene();

        rd->setColorClearValue(Color3(0.7f, 0.85f, 0.98f));
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
            rd->apply(m_skyShader, args);
        }
        rd->popState();

        // parts
        rd->pushState();
        rd->setDepthTest(RenderDevice::DEPTH_LESS);
        rd->setDepthWrite(true);
        rd->setCullFace(CullFace::BACK);
        drawMesh(rd, m_scene, vp, false);
        rd->popState();

        // selection box and handles (rebuilt every frame; they are tiny)
        MeshData box, handles;
        m_interaction.buildOverlays(box, handles, m_camera.position);
        if (!box.idx.empty()) {
            rd->pushState();
            rd->setDepthTest(RenderDevice::DEPTH_LEQUAL);
            rd->setCullFace(CullFace::BACK);
            drawMesh(rd, upload(box, VertexBuffer::WRITE_EVERY_FRAME), vp, true);
            rd->popState();
        }
        if (!handles.idx.empty()) {
            rd->pushState();
            rd->setDepthTest(RenderDevice::DEPTH_ALWAYS_PASS);
            rd->setDepthWrite(false);
            rd->setCullFace(CullFace::BACK);
            drawMesh(rd, upload(handles, VertexBuffer::WRITE_EVERY_FRAME), vp, true);
            rd->popState();
        }
    }

    // Keep G3D's own 2D overlay (profiler text, GUI) out of the picture.
    void onGraphics2D(RenderDevice*, Array<shared_ptr<Surface2D>>&) override {}

private:
    void rebuildScene() {
        MeshData m;
        std::vector<Part*> parts;
        m_editor.workspace()->collectParts(parts);
        for (const Part* p : parts) meshbuilder::addPart(m, *p);
        m_scene = upload(m, VertexBuffer::WRITE_ONCE);
        m_meshVersion = m_editor.version();
    }

    void drawMesh(RenderDevice* rd, const shared_ptr<GpuMesh>& g, const float vp[16], bool flat) {
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
        rd->apply(m_sceneShader, args);
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
    shared_ptr<GpuMesh> m_scene;
};

class ViewportImpl : public IViewport {
public:
    ViewportImpl(Editor& editor, const char* caption, int w, int h) : m_caption(caption) {
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

    bool frame() override { return m_app->step(); }

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
    std::string                m_caption;
    std::unique_ptr<StudioApp> m_app;
};

IViewport* createViewport(Editor& editor, const char* caption, int width, int height) {
    return new ViewportImpl(editor, caption, width, height);
}
