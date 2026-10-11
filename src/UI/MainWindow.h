#pragma once
#include <QMainWindow>
#include <QPlainTextEdit>
#include "DataModel/Editor.h"
#include "ViewportWidget.h"

class QAction;
class QActionGroup;
class QLabel;

// The classic Studio frame: menus, toolbars, Toolbox | viewport | Explorer + Properties, Output, status bar.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(Editor& editor, QWidget* parent = nullptr);
    ViewportWidget* viewport() const { return m_viewport; }

protected:
    void closeEvent(QCloseEvent* e) override;

private:
    void buildActions();
    void buildMenus();
    void buildToolbars();
    void buildDocks();
    void refreshActions();
    void refreshTitle();

    bool confirmDiscard();
    void fileNew();
    void fileOpen();
    bool fileSave();
    bool fileSaveAs();
    bool saveTo(const QString& path);
    void about();

    Editor&          m_editor;
    ViewportWidget*  m_viewport = nullptr;
    QPlainTextEdit*  m_output = nullptr;
    QLabel*          m_status = nullptr;
    class ToolboxPanel* m_toolbox = nullptr;

    QAction *aNew, *aOpen, *aSave, *aSaveAs, *aExit;
    QAction *aUndo, *aRedo, *aCut, *aCopy, *aPaste, *aDelete, *aDuplicate;
    QAction *aSelect, *aMove, *aResize;
    QAction *aRotate, *aTilt;
    QAction *aInsertBrick, *aInsertPlate, *aInsertCube;
    QAction *aAbout;
    QActionGroup* m_toolGroup = nullptr;
    QActionGroup* m_snapGroup = nullptr;
    std::vector<QAction*> m_dockToggles;
};
