#include <QScrollBar>
#include "ExplorerPanel.h"
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include "Icons.h"

ExplorerPanel::ExplorerPanel(Editor& editor, QWidget* parent) : QTreeWidget(parent), m_editor(editor) {
    setHeaderHidden(true);
    setIndentation(16);
    setEditTriggers(QAbstractItemView::EditKeyPressed);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setIconSize(QSize(16, 16));

    connect(this, &QTreeWidget::itemSelectionChanged, this, &ExplorerPanel::onItemSelectionChanged);
    connect(this, &QTreeWidget::itemChanged, this, &ExplorerPanel::onItemChanged);

    m_editor.treeChanged.connect([this]() { rebuild(); });
    m_editor.selectionChanged.connect([this]() { syncSelection(); });
    m_editor.partChanged.connect([this](Part* p) { onPartChanged(p); });

    rebuild();
}

Instance* ExplorerPanel::instanceOf(QTreeWidgetItem* item) const {
    if (!item) { return nullptr; }
    return reinterpret_cast<Instance*>(item->data(0, Qt::UserRole).value<quintptr>());
}

void ExplorerPanel::addInstance(Instance* inst, QTreeWidgetItem* parentItem) {
    QTreeWidgetItem* item = parentItem ? new QTreeWidgetItem(parentItem) : new QTreeWidgetItem(this);
    item->setText(0, QString::fromStdString(inst->name));
    item->setData(0, Qt::UserRole, QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(inst)));
    if (Part* part = dynamic_cast<Part*>(inst)) {
        item->setIcon(0, Icons::partIcon(part->brickColor));
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    } else {
        item->setIcon(0, Icons::icon(Icons::Workspace));
    }
    m_items[inst] = item;
    for (const auto& c : inst->children()) { addInstance(c.get(), item); }
}

void ExplorerPanel::rebuild() {
    m_busy = true;
    const int scroll = verticalScrollBar()->value();
    clear();
    m_items.clear();
    if (m_editor.workspace()) {
        addInstance(m_editor.workspace().get(), nullptr);
        expandAll();
    }
    m_busy = false;
    syncSelection();
    verticalScrollBar()->setValue(scroll);
}

void ExplorerPanel::syncSelection() {
    m_busy = true;
    clearSelection();
    if (Part* p = m_editor.selected()) {
        auto it = m_items.find(p);
        if (it != m_items.end()) {
            it->second->setSelected(true);
            setCurrentItem(it->second);
            scrollToItem(it->second);
        }
    }
    m_busy = false;
}

void ExplorerPanel::onItemSelectionChanged() {
    if (m_busy) { return; }
    const QList<QTreeWidgetItem*> sel = selectedItems();
    Part* part = sel.isEmpty() ? nullptr : dynamic_cast<Part*>(instanceOf(sel.first()));
    m_editor.select(part);
}

void ExplorerPanel::onItemChanged(QTreeWidgetItem* item, int) {
    if (m_busy) { return; }
    Part* part = dynamic_cast<Part*>(instanceOf(item));
    if (!part) { return; }
    const std::string newName = item->text(0).toStdString();
    if (newName == part->name || newName.empty()) {
        m_busy = true; item->setText(0, QString::fromStdString(part->name)); m_busy = false;
        return;
    }
    PartState s = part->state();
    s.name = newName;
    m_editor.setPartState(part, s, "Rename " + part->name);
}

void ExplorerPanel::onPartChanged(Part* part) {
    auto it = m_items.find(part);
    if (it == m_items.end()) { return; }
    m_busy = true;
    it->second->setText(0, QString::fromStdString(part->name));
    it->second->setIcon(0, Icons::partIcon(part->brickColor));
    m_busy = false;
}

void ExplorerPanel::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Delete && m_editor.selected()) {
        m_editor.deleteSelected();
        return;
    }
    if (e->key() == Qt::Key_F2 && currentItem() && dynamic_cast<Part*>(instanceOf(currentItem()))) {
        editItem(currentItem(), 0);
        return;
    }
    QTreeWidget::keyPressEvent(e);
}

void ExplorerPanel::contextMenuEvent(QContextMenuEvent* e) {
    QTreeWidgetItem* item = itemAt(e->pos());
    Part* part = dynamic_cast<Part*>(instanceOf(item));
    if (!part) { return; }
    m_editor.select(part);

    QMenu menu(this);
    QAction* rename = menu.addAction(tr("Rename"));
    QAction* dup = menu.addAction(Icons::icon(Icons::Copy), tr("Duplicate"));
    QAction* del = menu.addAction(Icons::icon(Icons::Delete), tr("Delete"));
    QAction* chosen = menu.exec(e->globalPos());
    if (chosen == rename) { editItem(item, 0); }
    else if (chosen == dup) { m_editor.duplicateSelected(); }
    else if (chosen == del) { m_editor.deleteSelected(); }
}
