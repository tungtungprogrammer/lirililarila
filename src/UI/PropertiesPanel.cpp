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
    m_color = m_topSurface = m_anchored = nullptr;
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

        QTreeWidgetItem* data = addCategory(tr("Data"));
        m_class = addTextRow(data, tr("ClassName"), true);
        m_name = addTextRow(data, tr("Name"));

        QTreeWidgetItem* behavior = addCategory(tr("Behavior"));
        m_anchored = new QComboBox;
        m_anchored->addItem("true", true);
        m_anchored->addItem("false", false);
        addRow(behavior, tr("Anchored"), m_anchored);

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
    m_busy = wasBusy;
}
