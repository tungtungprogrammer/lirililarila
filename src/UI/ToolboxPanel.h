#pragma once
#include <QListWidget>
#include "DataModel/Editor.h"

// The Toolbox: click an entry to insert a part in front of the camera.
class ToolboxPanel : public QListWidget {
    Q_OBJECT
public:
    ToolboxPanel(Editor& editor, QWidget* parent = nullptr);

    struct Preset { const char* label; const char* tip; Vec3 size; };
    static const std::vector<Preset>& presets();
    void insert(int index);

private:
    Editor& m_editor;
};
