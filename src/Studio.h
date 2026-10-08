#pragma once
#include <G3D/G3DAll.h>
#include "DataModel/Workspace.h"

class Studio : public GApp {
public:
    explicit Studio(const GApp::Settings& settings);

    void onInit() override;
    bool onEvent(const GEvent& e) override;
    void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) override;
    void onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& surface2D) override;

private:
    void buildDefaultPlace();
    void drawWorkspace(RenderDevice* rd);

    std::shared_ptr<Workspace> m_workspace;
    Part* m_selected = nullptr;   // non-owning; cleared when the part is deleted
};
