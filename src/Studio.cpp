#include "Studio.h"

Studio::Studio(const GApp::Settings& settings) : GApp(settings) {
    renderDevice->setColorClearValue(Color3(0.55f, 0.75f, 0.95f));   // sky blue
}

void Studio::buildDefaultPlace() {
    m_workspace = std::make_shared<Workspace>();
    m_workspace->createPart("Baseplate", Point3(0, -0.5f, 0), Vector3(512, 1, 512), Color3(0.38f, 0.60f, 0.20f));
    m_workspace->createPart("Brick1", Point3(0, 0.6f, 0),    Vector3(4, 1.2f, 2), Color3(0.77f, 0.16f, 0.11f));
    m_workspace->createPart("Brick2", Point3(6, 0.6f, 3),    Vector3(4, 1.2f, 2), Color3(0.05f, 0.41f, 0.67f));
    m_workspace->createPart("Brick3", Point3(-6, 0.6f, -3),  Vector3(4, 1.2f, 2), Color3(0.96f, 0.80f, 0.19f));
}

void Studio::onInit() {
    GApp::onInit();
    showRenderingStats = false;
    buildDefaultPlace();
    // Start camera (the debug controller drives the camera: right mouse + WASD)
    m_debugCamera->setPosition(Point3(12, 8, 12));
    m_debugCamera->lookAt(Point3(0, 1, 0));
    m_debugController->setFrame(m_debugCamera->frame());
}

bool Studio::onEvent(const GEvent& e) {
    if (GApp::onEvent(e)) { return true; }

    // Left click: select
    if (e.type == GEventType::MOUSE_BUTTON_DOWN && e.button.button == 0) {
        const Vector2 m = userInput->mouseXY();
        const Ray ray = activeCamera()->worldRay(m.x, m.y, renderDevice->viewport());
        m_selected = m_workspace->pick(ray);
        return true;
    }

    if (e.type == GEventType::KEY_DOWN) {
        // Delete: remove selected part
        if (e.key.keysym.sym == GKey::DELETE && m_selected) {
            Part* doomed = m_selected;
            m_selected = nullptr;
            doomed->destroy();
            return true;
        }
        // P: insert a new part in front of the camera
        if (e.key.keysym.sym == 'p') {
            const CFrame cam = activeCamera()->frame();
            m_workspace->createPart("Part", cam.translation + cam.lookVector() * 10.0f,
                                    Vector3(4, 1.2f, 2), Color3(0.64f, 0.64f, 0.64f));
            return true;
        }
    }
    return false;
}

void Studio::drawWorkspace(RenderDevice* rd) {
    std::vector<Part*> parts;
    m_workspace->collectParts(parts);
    for (Part* p : parts) {
        Draw::box(p->box(), rd, p->color, Color4::clear());
        if (p == m_selected) {
            Draw::box(p->box(), rd, Color4::clear(), Color3(0.1f, 0.5f, 1.0f));
        }
    }
}

void Studio::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) {
    // The framework has already set up the camera; just clear and draw (same as the G3D tinyStarter sample).
    rd->swapBuffers();
    rd->clear();
    drawWorkspace(rd);
    drawDebugShapes();
}

void Studio::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& surface2D) {
    Surface2D::sortAndRender(rd, surface2D);
}
