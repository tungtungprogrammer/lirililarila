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
        {"Spawn",  "Where the player appears in Play Solo", Vec3(6, 1, 6), PartShape::Block, kBrickMediumStoneGrey, true, 0, PartBehavior::None, 0, "SpawnLocation"},
        {"Ramp",   "Walkable ramp, 8 wide, 6 high, 12 long", Vec3(8, 6, 12), PartShape::Wedge, kBrickMediumStoneGrey, true},
        {"Lava",   "Kills the player on touch", Vec3(10, 0.4f, 10), PartShape::Block, kBrickBrightRed, true, 0, PartBehavior::Kill, 0, "Lava"},
        {"JumpPad", "Launches the player upward", Vec3(4, 0.4f, 4), PartShape::Block, kBrickBrightGreen, true, 0, PartBehavior::JumpPad, 100, "JumpPad"},
        {"Conveyor", "Carries the player along its -Z direction", Vec3(4, 0.4f, 12), PartShape::Block, kBrickMediumStoneGrey, true, 0, PartBehavior::Conveyor, 14, "Conveyor"},
        {"Spinner", "A bar that turns around; ride it or get knocked off", Vec3(16, 1, 1.5f), PartShape::Block, kBrickBrightYellow, true, 0, PartBehavior::Spin, 60, "Spinner"},
        {"Glass",  "See-through wall", Vec3(8, 8, 0.5f), PartShape::Block, kBrickBrightBlue, true, 0.5f},
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
    PartState s;
    s.name = p.partName ? p.partName : (p.shape == PartShape::Block ? "Part" : p.label);
    s.size = p.size;
    s.shape = p.shape;
    s.brickColor = p.color;
    s.anchored = p.anchored;
    s.transparency = p.transparency;
    s.behavior = p.behavior;
    s.behaviorValue = p.value;
    m_editor.insertPartFrom(s);
}
