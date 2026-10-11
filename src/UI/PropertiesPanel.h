#pragma once
#include <QComboBox>
#include <QLineEdit>
#include <QTreeWidget>
#include "DataModel/Editor.h"

// The Properties window: grouped, editable properties of the selected Part (text vectors, like the original).
class PropertiesPanel : public QTreeWidget {
    Q_OBJECT
public:
    PropertiesPanel(Editor& editor, QWidget* parent = nullptr);

private:
    void rebuild();
    void refreshValues();
    QTreeWidgetItem* addCategory(const QString& title);
    QTreeWidgetItem* addRow(QTreeWidgetItem* category, const QString& label, QWidget* editor);
    QLineEdit* addTextRow(QTreeWidgetItem* category, const QString& label, bool readOnly = false);

    void commit(const std::function<void(PartState&)>& mutate, const QString& what);
    static QString vecText(const Vec3& v);
    static bool parseVec(const QString& text, Vec3& out);

    Editor& m_editor;
    Part*   m_part = nullptr;
    bool    m_busy = false;

    QLineEdit* m_class = nullptr;
    QLineEdit* m_name = nullptr;
    QLineEdit* m_position = nullptr;
    QLineEdit* m_rotation = nullptr;
    QLineEdit* m_size = nullptr;
    QComboBox* m_color = nullptr;
    QComboBox* m_topSurface = nullptr;
    QComboBox* m_anchored = nullptr;
};
