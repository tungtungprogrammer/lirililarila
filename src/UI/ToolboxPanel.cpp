#include "ToolboxPanel.h"
#include "Icons.h"

const std::vector<ToolboxPanel::Preset>& ToolboxPanel::presets() {
    static const std::vector<Preset> list = {
        {"Brick",  "Classic 4x2 brick",         Vec3(4, 1.2f, 2)},
        {"Plate",  "Thin 4x2 plate",            Vec3(4, 0.4f, 2)},
        {"Cube",   "2x2x2 block",               Vec3(2, 2, 2)},
        {"Wall",   "Wall 12 studs wide",        Vec3(12, 6, 1)},
        {"Pillar", "Tall 2x2 pillar",           Vec3(2, 8, 2)},
        {"Floor",  "Large flat 16x16 floor",    Vec3(16, 1, 16)},
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
    m_editor.insertPart("Part", p.size, kBrickMediumStoneGrey);
}
