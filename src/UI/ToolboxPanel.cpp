#include "ToolboxPanel.h"
#include "Icons.h"

const std::vector<ToolboxPanel::Preset>& ToolboxPanel::presets() {
    static const std::vector<Preset> list = {
        {"Brick",  "Classic 4x2 brick",         Vec3(4, 1.2f, 2), PartShape::Block},
        {"Plate",  "Thin 4x2 plate",            Vec3(4, 0.4f, 2), PartShape::Block},
        {"Cube",   "2x2x2 block",               Vec3(2, 2, 2), PartShape::Block},
        {"Wall",   "Wall 12 studs wide",        Vec3(12, 6, 1), PartShape::Block},
        {"Pillar", "Tall 2x2 pillar",           Vec3(2, 8, 2), PartShape::Block},
        {"Floor",  "Large flat 16x16 floor",    Vec3(16, 1, 16), PartShape::Block},
        {"Ball",   "Sphere, 4 studs across",    Vec3(4, 4, 4), PartShape::Ball},
        {"Cylinder", "Cylinder lying along X",  Vec3(4, 2, 2), PartShape::Cylinder},
        {"Wedge",  "Ramp / slope",              Vec3(4, 2, 4), PartShape::Wedge},
    };
    return list;
}

ToolboxPanel::ToolboxPanel(Editor& editor, QWidget* parent) : QListWidget(parent), m_editor(editor) {
    setIconSize(QSize(24, 24));
    setSpacing(1);
    setUniformItemSizes(true);
    int i = 0;
    for (const Preset& p : presets()) {
        auto* item = new QListWidgetItem(Icons::partIcon(kBrickMediumStoneGrey), QString::fromLatin1(p.label), this);
        item->setToolTip(QString::fromLatin1(p.tip) + QString(" (%1 x %2 x %3)").arg(p.size.x).arg(p.size.y).arg(p.size.z));
        item->setData(Qt::UserRole, i++);
    }
    connect(this, &QListWidget::itemClicked, this, [this](QListWidgetItem* it) { insert(it->data(Qt::UserRole).toInt()); });
}

void ToolboxPanel::insert(int index) {
    if (index < 0 || index >= static_cast<int>(presets().size())) { return; }
    const Preset& p = presets()[static_cast<std::size_t>(index)];
    m_editor.insertPart(p.shape == PartShape::Block ? "Part" : p.label, p.size, kBrickMediumStoneGrey, p.shape);
}
