#include "PropertiesPanel.h"
#include <QHeaderView>
#include <QRegularExpression>
#include "Icons.h"

namespace {
QString num(float v) { return QString::number(static_cast<double>(v), 'g', 6); }
}

QString PropertiesPanel::vecText(const Vec3& v) { return num(v.x) + ", " + num(v.y) + ", " + num(v.z); }

bool PropertiesPanel::parseVec(const QString& text, Vec3& out) {
    const QStringList parts = text.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
    if (parts.size() != 3) { return false; }
    float v[3];
    for (int i = 0; i < 3; ++i) {
        bool ok = false;
        v[i] = parts[i].toFloat(&ok);
        if (!ok) { return false; }
    }
    out = Vec3(v[0], v[1], v[2]);
    return true;
}

PropertiesPanel::PropertiesPanel(Editor& editor, QWidget* parent) : QTreeWidget(parent), m_editor(editor) {
    setColumnCount(2);
    setHeaderHidden(true);
    setRootIsDecorated(true);
    setIndentation(12);
    setSelectionMode(QAbstractItemView::NoSelection);
    setAlternatingRowColors(false);
    header()->setSectionResizeMode(0, QHeaderView::Interactive);
    header()->setStretchLastSection(true);
    setColumnWidth(0, 105);

    m_editor.selectionChanged.connect([this]() { rebuild(); });
    m_editor.partChanged.connect([this](Part* p) { if (p == m_part) { refreshValues(); } });
    m_editor.treeChanged.connect([this]() { if (m_part && m_editor.selected() != m_part) { rebuild(); } });
    m_editor.historyChanged.connect([this]() { refreshValues(); });
    rebuild();
}

QTreeWidgetItem* PropertiesPanel::addCategory(const QString& title) {
    auto* item = new QTreeWidgetItem(this);
    item->setText(0, title);
    QFont f = font(); f.setBold(true);
    item->setFont(0, f);
    item->setFirstColumnSpanned(true);
    item->setBackground(0, QColor(214, 210, 192));
    item->setBackground(1, QColor(214, 210, 192));
    item->setFlags(Qt::ItemIsEnabled);
    return item;
}

QTreeWidgetItem* PropertiesPanel::addRow(QTreeWidgetItem* category, const QString& label, QWidget* editor) {
    auto* item = new QTreeWidgetItem(category);
    item->setText(0, label);
    item->setSizeHint(0, QSize(0, 20));
    item->setFlags(Qt::ItemIsEnabled);
    setItemWidget(item, 1, editor);
    return item;
}

QLineEdit* PropertiesPanel::addTextRow(QTreeWidgetItem* category, const QString& label, bool readOnly) {
    auto* e = new QLineEdit;
    e->setFrame(false);
    e->setReadOnly(readOnly);
    if (readOnly) { e->setStyleSheet("background: transparent; color: #444;"); }
    addRow(category, label, e);
    return e;
}

void PropertiesPanel::commit(const std::function<void(PartState&)>& mutate, const QString& what) {
    if (m_busy || !m_part) { return; }
    PartState s = m_part->state();
    mutate(s);
    m_editor.setPartState(m_part, s, (what + " " + QString::fromStdString(s.name)).toStdString());
    refreshValues();   // snaps the text back if the value was rejected / normalised
}

void PropertiesPanel::rebuild() {
    m_busy = true;
    clear();
    m_class = m_name = m_position = m_rotation = m_size = nullptr;
    m_color = m_topSurface = m_anchored = m_shape = m_locked = m_canCollide = m_behavior = nullptr;
    m_behaviorValue = nullptr;
    m_transparency = nullptr;
    m_part = m_editor.selected();

    if (m_part) {
        QTreeWidgetItem* appearance = addCategory(tr("Appearance"));
        m_color = new QComboBox;
        for (const BrickColorInfo& c : brickColors()) {
            m_color->addItem(QIcon(Icons::swatch(c.id)), QString::fromLatin1(c.name), c.id);
        }
        addRow(appearance, tr("BrickColor"), m_color);
        m_topSurface = new QComboBox;
        m_topSurface->addItem("Studs", static_cast<int>(SurfaceType::Studs));
        m_topSurface->addItem("Smooth", static_cast<int>(SurfaceType::Smooth));
        addRow(appearance, tr("TopSurface"), m_topSurface);
        m_shape = new QComboBox;
        m_shape->addItem("Block", static_cast<int>(PartShape::Block));
        m_shape->addItem("Ball", static_cast<int>(PartShape::Ball));
        m_shape->addItem("Cylinder", static_cast<int>(PartShape::Cylinder));
        m_shape->addItem("Wedge", static_cast<int>(PartShape::Wedge));
        addRow(appearance, tr("Shape"), m_shape);
        m_transparency = addTextRow(appearance, tr("Transparency"));

        QTreeWidgetItem* data = addCategory(tr("Data"));
        m_class = addTextRow(data, tr("ClassName"), true);
        m_name = addTextRow(data, tr("Name"));

        QTreeWidgetItem* behavior = addCategory(tr("Behavior"));
        m_anchored = new QComboBox;
        m_anchored->addItem("true", true);
        m_anchored->addItem("false", false);
        addRow(behavior, tr("Anchored"), m_anchored);
        m_locked = new QComboBox;
        m_locked->addItem("false", false);
        m_locked->addItem("true", true);
        addRow(behavior, tr("Locked"), m_locked);
        m_canCollide = new QComboBox;
        m_canCollide->addItem("true", true);
        m_canCollide->addItem("false", false);
        addRow(behavior, tr("CanCollide"), m_canCollide);
        m_behavior = new QComboBox;
        m_behavior->addItem("None", static_cast<int>(PartBehavior::None));
        m_behavior->addItem("Kill (lava)", static_cast<int>(PartBehavior::Kill));
        m_behavior->addItem("JumpPad", static_cast<int>(PartBehavior::JumpPad));
        m_behavior->addItem("Conveyor", static_cast<int>(PartBehavior::Conveyor));
        m_behavior->addItem("Spin", static_cast<int>(PartBehavior::Spin));
        addRow(behavior, tr("Behavior"), m_behavior);
        m_behaviorValue = addTextRow(behavior, tr("BehaviorValue"));
        m_behaviorValue->setToolTip(tr("JumpPad: jump speed (0 = 90)\nConveyor: belt speed (0 = 16)\nSpin: degrees per second (0 = 60)"));

        QTreeWidgetItem* part = addCategory(tr("Part"));
        m_position = addTextRow(part, tr("Position"));
        m_rotation = addTextRow(part, tr("Rotation"));
        m_size = addTextRow(part, tr("Size"));

        connect(m_name, &QLineEdit::editingFinished, this, [this]() {
            const std::string n = m_name->text().toStdString();
            if (!n.empty()) { commit([&](PartState& s) { s.name = n; }, tr("Rename")); }
        });
        connect(m_position, &QLineEdit::editingFinished, this, [this]() {
            Vec3 v; if (parseVec(m_position->text(), v)) { commit([&](PartState& s) { s.position = v; }, tr("Move")); }
            else { m_editor.log("Position needs three numbers, e.g. 0, 5, 0"); refreshValues(); }
        });
        connect(m_rotation, &QLineEdit::editingFinished, this, [this]() {
            Vec3 v; if (parseVec(m_rotation->text(), v)) {
                commit([&](PartState& s) { s.orientation = Mat3::fromEulerYXZ(v.y, v.x, v.z); }, tr("Rotate"));
            } else { m_editor.log("Rotation needs three numbers (degrees), e.g. 0, 90, 0"); refreshValues(); }
        });
        connect(m_size, &QLineEdit::editingFinished, this, [this]() {
            Vec3 v; if (parseVec(m_size->text(), v)) {
                v = Vec3(std::max(0.2f, v.x), std::max(0.2f, v.y), std::max(0.2f, v.z));
                commit([&](PartState& s) { s.size = v; }, tr("Resize"));
            } else { m_editor.log("Size needs three numbers, e.g. 4, 1.2, 2"); refreshValues(); }
        });
        connect(m_color, QOverload<int>::of(&QComboBox::activated), this, [this](int i) {
            const int id = m_color->itemData(i).toInt();
            commit([&](PartState& s) { s.brickColor = id; }, tr("Recolor"));
        });
        connect(m_topSurface, QOverload<int>::of(&QComboBox::activated), this, [this](int i) {
            const SurfaceType sf = static_cast<SurfaceType>(m_topSurface->itemData(i).toInt());
            commit([&](PartState& s) { s.topSurface = sf; }, tr("Change TopSurface of"));
        });
        connect(m_shape, QOverload<int>::of(&QComboBox::activated), this, [this](int i) {
            const PartShape sh = static_cast<PartShape>(m_shape->itemData(i).toInt());
            commit([&](PartState& s) { s.shape = sh; }, tr("Change Shape of"));
        });
        connect(m_locked, QOverload<int>::of(&QComboBox::activated), this, [this](int i) {
            const bool l = m_locked->itemData(i).toBool();
            commit([&](PartState& s) { s.locked = l; }, l ? tr("Lock") : tr("Unlock"));
        });
        connect(m_transparency, &QLineEdit::editingFinished, this, [this]() {
            bool ok = false;
            float t = m_transparency->text().toFloat(&ok);
            if (!ok) { m_editor.log("Transparency needs a number from 0 to 1"); refreshValues(); return; }
            t = std::floor(std::max(0.0f, std::min(1.0f, t)) * 10.0f + 0.5f) / 10.0f;   // tenths
            commit([&](PartState& s) { s.transparency = t; }, tr("Change Transparency of"));
        });
        connect(m_canCollide, QOverload<int>::of(&QComboBox::activated), this, [this](int i) {
            const bool c = m_canCollide->itemData(i).toBool();
            commit([&](PartState& s) { s.canCollide = c; }, tr("Change CanCollide of"));
        });
        connect(m_behavior, QOverload<int>::of(&QComboBox::activated), this, [this](int i) {
            const PartBehavior b = static_cast<PartBehavior>(m_behavior->itemData(i).toInt());
            commit([&](PartState& s) { s.behavior = b; }, tr("Change Behavior of"));
        });
        connect(m_behaviorValue, &QLineEdit::editingFinished, this, [this]() {
            bool ok = false;
            const float v = m_behaviorValue->text().toFloat(&ok);
            if (!ok) { m_editor.log("BehaviorValue needs a number"); refreshValues(); return; }
            commit([&](PartState& s) { s.behaviorValue = v; }, tr("Change BehaviorValue of"));
        });
        connect(m_anchored, QOverload<int>::of(&QComboBox::activated), this, [this](int i) {
            const bool a = m_anchored->itemData(i).toBool();
            commit([&](PartState& s) { s.anchored = a; }, tr("Anchor"));
        });

        expandAll();
    }
    m_busy = false;
    refreshValues();
}

void PropertiesPanel::refreshValues() {
    if (!m_part) { return; }
    const bool wasBusy = m_busy;
    m_busy = true;
    const PartState s = m_part->state();

    auto setText = [](QLineEdit* e, const QString& t) { if (e && !e->hasFocus()) { e->setText(t); } };
    setText(m_class, "Part");
    setText(m_name, QString::fromStdString(s.name));
    setText(m_position, vecText(s.position));
    setText(m_size, vecText(s.size));
    float yaw, pitch, roll;
    s.orientation.toEulerYXZ(yaw, pitch, roll);
    setText(m_rotation, vecText(Vec3(pitch, yaw, roll)));

    if (m_color) { m_color->setCurrentIndex(std::max(0, m_color->findData(s.brickColor))); }
    if (m_topSurface) { m_topSurface->setCurrentIndex(std::max(0, m_topSurface->findData(static_cast<int>(s.topSurface)))); }
    if (m_anchored) { m_anchored->setCurrentIndex(s.anchored ? 0 : 1); }
    if (m_locked) { m_locked->setCurrentIndex(s.locked ? 1 : 0); }
    if (m_canCollide) { m_canCollide->setCurrentIndex(s.canCollide ? 0 : 1); }
    if (m_behavior) { m_behavior->setCurrentIndex(std::max(0, m_behavior->findData(static_cast<int>(s.behavior)))); }
    setText(m_behaviorValue, num(s.behaviorValue));
    if (m_shape) { m_shape->setCurrentIndex(std::max(0, m_shape->findData(static_cast<int>(s.shape)))); }
    setText(m_transparency, num(s.transparency));
    m_busy = wasBusy;
}
