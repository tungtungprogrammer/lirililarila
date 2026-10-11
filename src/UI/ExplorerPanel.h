#pragma once
#include <QTreeWidget>
#include <map>
#include "DataModel/Editor.h"

// The Explorer window: a tree of the place's Instances, synchronised with the selection.
class ExplorerPanel : public QTreeWidget {
    Q_OBJECT
public:
    ExplorerPanel(Editor& editor, QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* e) override;

private:
    void rebuild();
    void addInstance(Instance* inst, QTreeWidgetItem* parentItem);
    void syncSelection();
    void onItemSelectionChanged();
    void onItemChanged(QTreeWidgetItem* item, int column);
    void onPartChanged(Part* part);
    Instance* instanceOf(QTreeWidgetItem* item) const;

    Editor& m_editor;
    std::map<const Instance*, QTreeWidgetItem*> m_items;
    bool m_busy = false;
};
