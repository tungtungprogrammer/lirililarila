#include "MainWindow.h"

#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>

#include "ExplorerPanel.h"
#include "Icons.h"
#include "PlaceFile.h"
#include "PropertiesPanel.h"
#include "ToolboxPanel.h"

static const char* kFilter = "OldBlox Place (*.obxl);;All Files (*.*)";

MainWindow::MainWindow(Editor& editor, QWidget* parent) : QMainWindow(parent), m_editor(editor) {
    setDockNestingEnabled(true);
    m_viewport = new ViewportWidget(this);
    setCentralWidget(m_viewport);

    buildActions();
    buildDocks();
    buildMenus();
    buildToolbars();

    m_status = new QLabel("Ready", this);
    statusBar()->addWidget(m_status, 1);
    m_info = new QLabel(this);
    statusBar()->addPermanentWidget(m_info);
    statusBar()->setSizeGripEnabled(true);

    m_editor.titleChanged.connect([this] { refreshTitle(); });
    m_editor.historyChanged.connect([this] { refreshActions(); });
    m_editor.selectionChanged.connect([this] { refreshActions(); });
    m_editor.toolChanged.connect([this] { refreshActions(); });
    m_editor.snapChanged.connect([this] { refreshActions(); });
    m_editor.lightingChanged.connect([this] { refreshActions(); });
    m_editor.treeChanged.connect([this] { refreshActions(); });
    m_editor.partChanged.connect([this](Part*) { refreshActions(); });
    m_editor.output.connect([this](const std::string& s) {
        m_output->appendPlainText(QString::fromStdString(s));
    });

    refreshActions();
    refreshTitle();
    resize(1180, 760);
}

void MainWindow::buildActions() {
    auto mk = [this](const QString& text, Icons::Id icon, const QKeySequence& key, bool hasIcon = true) {
        auto* a = new QAction(text, this);
        if (hasIcon) a->setIcon(Icons::icon(icon));
        if (!key.isEmpty()) a->setShortcut(key);
        return a;
    };

    aNew = mk("&New", Icons::New, QKeySequence::New);
    aOpen = mk("&Open...", Icons::Open, QKeySequence::Open);
    aSave = mk("&Save", Icons::Save, QKeySequence::Save);
    aSaveAs = mk("Save &As...", Icons::Save, QKeySequence(), false);
    aExit = mk("E&xit", Icons::New, QKeySequence(), false);

    aUndo = mk("&Undo", Icons::Undo, QKeySequence::Undo);
    aRedo = mk("&Redo", Icons::Redo, QKeySequence::Redo);
    aCut = mk("Cu&t", Icons::Cut, QKeySequence::Cut);
    aCopy = mk("&Copy", Icons::Copy, QKeySequence::Copy);
    aPaste = mk("&Paste", Icons::Paste, QKeySequence::Paste);
    aDelete = mk("&Delete", Icons::Delete, QKeySequence::Delete);
    aDuplicate = mk("D&uplicate", Icons::Copy, QKeySequence(Qt::CTRL | Qt::Key_D), false);

    aSelect = mk("&Select", Icons::Select, QKeySequence(Qt::Key_1));
    aMove = mk("&Move", Icons::Move, QKeySequence(Qt::Key_2));
    aResize = mk("&Resize", Icons::Resize, QKeySequence(Qt::Key_3));
    for (QAction* a : {aSelect, aMove, aResize}) a->setCheckable(true);
    m_toolGroup = new QActionGroup(this);
    for (QAction* a : {aSelect, aMove, aResize}) m_toolGroup->addAction(a);

    aRotate = mk("&Rotate 90\xC2\xB0 (Y)", Icons::Move, QKeySequence(Qt::CTRL | Qt::Key_R), false);
    aTilt = mk("&Tilt 90\xC2\xB0 (Z)", Icons::Move, QKeySequence(Qt::CTRL | Qt::Key_T), false);

    aInsertBrick = mk("&Brick", Icons::Brick, QKeySequence());
    aInsertPlate = mk("&Plate", Icons::Brick, QKeySequence(), false);
    aInsertCube = mk("&Cube", Icons::Brick, QKeySequence(), false);
    aAnchor = mk("&Anchor / Unanchor", Icons::Move, QKeySequence(Qt::CTRL | Qt::Key_B), false);
    aLock = mk("&Lock / Unlock", Icons::Move, QKeySequence(Qt::CTRL | Qt::Key_L), false);
    aResetCamera = mk("&Reset Camera", Icons::Select, QKeySequence(Qt::CTRL | Qt::Key_Home), false);
    aControls = mk("&Controls", Icons::About, QKeySequence(Qt::Key_F1), false);
    aAbout = mk("&About OldBlox Studio", Icons::About, QKeySequence(), false);

    connect(aNew, &QAction::triggered, this, &MainWindow::fileNew);
    connect(aOpen, &QAction::triggered, this, &MainWindow::fileOpen);
    connect(aSave, &QAction::triggered, this, [this] { fileSave(); });
    connect(aSaveAs, &QAction::triggered, this, [this] { fileSaveAs(); });
    connect(aExit, &QAction::triggered, this, &QWidget::close);
    connect(aUndo, &QAction::triggered, this, [this] { m_editor.undo(); });
    connect(aRedo, &QAction::triggered, this, [this] { m_editor.redo(); });
    connect(aCut, &QAction::triggered, this, [this] { m_editor.cutSelected(); });
    connect(aCopy, &QAction::triggered, this, [this] { m_editor.copySelected(); refreshActions(); });
    connect(aPaste, &QAction::triggered, this, [this] { m_editor.paste(); });
    connect(aDelete, &QAction::triggered, this, [this] { m_editor.deleteSelected(); });
    connect(aDuplicate, &QAction::triggered, this, [this] { m_editor.duplicateSelected(); });
    connect(aSelect, &QAction::triggered, this, [this] { m_editor.setTool(Tool::Select); });
    connect(aMove, &QAction::triggered, this, [this] { m_editor.setTool(Tool::Move); });
    connect(aResize, &QAction::triggered, this, [this] { m_editor.setTool(Tool::Resize); });
    connect(aRotate, &QAction::triggered, this, [this] { m_editor.rotateSelected(1); });
    connect(aTilt, &QAction::triggered, this, [this] { m_editor.rotateSelected(2); });
    connect(aInsertBrick, &QAction::triggered, this, [this] { m_toolbox->insert(0); });
    connect(aInsertPlate, &QAction::triggered, this, [this] { m_toolbox->insert(1); });
    connect(aInsertCube, &QAction::triggered, this, [this] { m_toolbox->insert(2); });
    connect(aAnchor, &QAction::triggered, this, [this] { m_editor.toggleAnchorSelected(); });
    connect(aLock, &QAction::triggered, this, [this] { m_editor.toggleLockSelected(); });
    connect(aResetCamera, &QAction::triggered, this, [this] { m_editor.requestCameraReset(); });
    connect(aControls, &QAction::triggered, this, &MainWindow::controls);
    connect(aAbout, &QAction::triggered, this, &MainWindow::about);
}

void MainWindow::buildDocks() {
    auto dock = [this](const QString& title, QWidget* w, Qt::DockWidgetArea area, const QString& objName) {
        auto* d = new QDockWidget(title, this);
        d->setObjectName(objName);
        d->setWidget(w);
        d->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        addDockWidget(area, d);
        m_dockToggles.push_back(d->toggleViewAction());
        return d;
    };

    m_toolbox = new ToolboxPanel(m_editor, this);
    auto* tb = dock("Toolbox", m_toolbox, Qt::LeftDockWidgetArea, "toolbox");
    auto* ex = dock("Explorer", new ExplorerPanel(m_editor, this), Qt::RightDockWidgetArea, "explorer");
    auto* pr = dock("Properties", new PropertiesPanel(m_editor, this), Qt::RightDockWidgetArea, "properties");

    m_output = new QPlainTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setMaximumBlockCount(2000);
    m_output->setFont(QFont("Courier New", 9));
    auto* out = dock("Output", m_output, Qt::BottomDockWidgetArea, "output");

    tb->setMinimumWidth(130);
    resizeDocks({tb}, {150}, Qt::Horizontal);
    resizeDocks({ex, pr}, {260, 260}, Qt::Vertical);
    resizeDocks({ex}, {250}, Qt::Horizontal);
    resizeDocks({out}, {110}, Qt::Vertical);
}

void MainWindow::buildMenus() {
    QMenu* file = menuBar()->addMenu("&File");
    file->addAction(aNew);
    file->addAction(aOpen);
    file->addSeparator();
    file->addAction(aSave);
    file->addAction(aSaveAs);
    file->addSeparator();
    file->addAction(aExit);

    QMenu* edit = menuBar()->addMenu("&Edit");
    edit->addAction(aUndo);
    edit->addAction(aRedo);
    edit->addSeparator();
    edit->addAction(aCut);
    edit->addAction(aCopy);
    edit->addAction(aPaste);
    edit->addSeparator();
    edit->addAction(aDuplicate);
    edit->addAction(aDelete);

    QMenu* view = menuBar()->addMenu("&View");
    for (QAction* a : m_dockToggles) view->addAction(a);
    view->addSeparator();
    view->addAction(aResetCamera);
    QMenu* light = view->addMenu("&Lighting (Time of Day)");
    m_lightGroup = new QActionGroup(this);
    int li = 0;
    for (const char* name : {"&Day", "&Sunset", "&Night"}) {
        QAction* a = light->addAction(name);
        a->setCheckable(true);
        a->setData(li);
        m_lightGroup->addAction(a);
        const Lighting l = static_cast<Lighting>(li++);
        connect(a, &QAction::triggered, this, [this, l] { m_editor.setLighting(l); });
    }

    QMenu* insert = menuBar()->addMenu("&Insert");
    insert->addAction(aInsertBrick);
    insert->addAction(aInsertPlate);
    insert->addAction(aInsertCube);
    insert->addSeparator();
    for (std::size_t i = 3; i < ToolboxPanel::presets().size(); ++i) {
        const ToolboxPanel::Preset& pr = ToolboxPanel::presets()[i];
        QAction* a = insert->addAction(QString::fromLatin1(pr.label));
        a->setToolTip(QString::fromLatin1(pr.tip));
        connect(a, &QAction::triggered, this, [this, i] { m_toolbox->insert(static_cast<int>(i)); });
    }

    QMenu* format = menuBar()->addMenu("F&ormat");
    format->addAction(aRotate);
    format->addAction(aTilt);
    format->addSeparator();
    format->addAction(aAnchor);
    format->addAction(aLock);
    format->addSeparator();
    QMenu* paint = format->addMenu("&Color");
    for (const BrickColorInfo& c : brickColors()) {
        QAction* a = paint->addAction(QIcon(Icons::swatch(c.id)), QString::fromLatin1(c.name));
        const int id = c.id;
        connect(a, &QAction::triggered, this, [this, id] { m_editor.colorSelected(id); });
    }

    QMenu* tools = menuBar()->addMenu("&Tools");
    tools->addAction(aSelect);
    tools->addAction(aMove);
    tools->addAction(aResize);
    tools->addSeparator();
    QMenu* snap = tools->addMenu("&Grid Snap");
    m_snapGroup = new QActionGroup(this);
    struct Opt { const char* label; float value; };
    for (const Opt& o : {Opt{"Off", 0.f}, Opt{"1/2 Stud", 0.5f}, Opt{"1 Stud", 1.f}, Opt{"2 Studs", 2.f}, Opt{"4 Studs", 4.f}}) {
        QAction* a = snap->addAction(o.label);
        a->setCheckable(true);
        a->setData(o.value);
        m_snapGroup->addAction(a);
        const float v = o.value;
        connect(a, &QAction::triggered, this, [this, v] { m_editor.setSnap(v); });
    }

    QMenu* help = menuBar()->addMenu("&Help");
    help->addAction(aControls);
    help->addSeparator();
    help->addAction(aAbout);
}

void MainWindow::buildToolbars() {
    auto* std_ = addToolBar("Standard");
    std_->setObjectName("standardBar");
    std_->setMovable(false);
    for (QAction* a : {aNew, aOpen, aSave}) std_->addAction(a);
    std_->addSeparator();
    for (QAction* a : {aCut, aCopy, aPaste}) std_->addAction(a);
    std_->addSeparator();
    for (QAction* a : {aUndo, aRedo}) std_->addAction(a);

    auto* tools = addToolBar("Tools");
    tools->setObjectName("toolsBar");
    tools->setMovable(false);
    for (QAction* a : {aSelect, aMove, aResize}) tools->addAction(a);
    tools->addSeparator();
    tools->addAction(aInsertBrick);
    tools->addAction(aDelete);
}

void MainWindow::refreshActions() {
    const bool hasSel = m_editor.selected() != nullptr;
    aUndo->setEnabled(m_editor.canUndo());
    aRedo->setEnabled(m_editor.canRedo());
    aUndo->setText(m_editor.canUndo() ? QString("&Undo %1").arg(QString::fromStdString(m_editor.undoText())) : "&Undo");
    aRedo->setText(m_editor.canRedo() ? QString("&Redo %1").arg(QString::fromStdString(m_editor.redoText())) : "&Redo");
    for (QAction* a : {aCut, aCopy, aDelete, aDuplicate, aRotate, aTilt, aAnchor, aLock}) a->setEnabled(hasSel);
    aPaste->setEnabled(m_editor.hasClipboard());

    aSelect->setChecked(m_editor.tool() == Tool::Select);
    aMove->setChecked(m_editor.tool() == Tool::Move);
    aResize->setChecked(m_editor.tool() == Tool::Resize);

    for (QAction* a : m_snapGroup->actions())
        a->setChecked(qFuzzyCompare(a->data().toFloat() + 1.f, m_editor.snap() + 1.f));

    for (QAction* a : m_lightGroup->actions()) a->setChecked(a->data().toInt() == static_cast<int>(m_editor.lighting()));

    static const char* kShapeNames[] = {"Block", "Ball", "Cylinder", "Wedge"};
    static const char* kToolNames[] = {"Select", "Move", "Resize"};
    if (Part* p = m_editor.selected()) {
        m_status->setText(QString("%1 (%2)  size %3 x %4 x %5%6%7")
                              .arg(QString::fromStdString(p->name), kShapeNames[static_cast<int>(p->shape)])
                              .arg(static_cast<double>(p->size.x), 0, 'g', 4).arg(static_cast<double>(p->size.y), 0, 'g', 4)
                              .arg(static_cast<double>(p->size.z), 0, 'g', 4)
                              .arg(p->anchored ? "  [anchored]" : "", p->locked ? "  [locked]" : ""));
    } else {
        m_status->setText("Ready");
    }
    if (m_info)
        m_info->setText(QString("%1 parts   Tool: %2   Snap: %3")
                            .arg(m_editor.partCount())
                            .arg(kToolNames[static_cast<int>(m_editor.tool())])
                            .arg(m_editor.snap() > 0 ? QString::number(static_cast<double>(m_editor.snap())) + " stud" : QString("off")));
}

void MainWindow::refreshTitle() {
    const QString path = QString::fromStdString(m_editor.filePath());
    const QString name = path.isEmpty() ? QString("Place1") : QFileInfo(path).fileName();
    setWindowTitle(QString("%1%2 - OldBlox Studio").arg(name, m_editor.modified() ? "*" : ""));
}

bool MainWindow::confirmDiscard() {
    if (!m_editor.modified()) return true;
    QMessageBox box(QMessageBox::Warning, "OldBlox Studio", "Do you want to save changes to this place?",
                    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, this);
    const int r = box.exec();
    if (r == QMessageBox::Save) return fileSave();
    return r == QMessageBox::Discard;
}

void MainWindow::fileNew() {
    if (confirmDiscard()) m_editor.newPlace();
}

void MainWindow::fileOpen() {
    if (!confirmDiscard()) return;
    const QString path = QFileDialog::getOpenFileName(this, "Open Place", QString(), kFilter);
    if (path.isEmpty()) return;
    QString err;
    if (!PlaceFile::load(m_editor, path, &err))
        QMessageBox::warning(this, "OldBlox Studio", "Could not open the file:\n" + err);
}

bool MainWindow::saveTo(const QString& path) {
    QString err;
    if (!PlaceFile::save(m_editor, path, &err)) {
        QMessageBox::warning(this, "OldBlox Studio", "Could not save the file:\n" + err);
        return false;
    }
    return true;
}

bool MainWindow::fileSave() {
    if (m_editor.filePath().empty()) return fileSaveAs();
    return saveTo(QString::fromStdString(m_editor.filePath()));
}

bool MainWindow::fileSaveAs() {
    QString path = QFileDialog::getSaveFileName(this, "Save Place As", QString(), kFilter);
    if (path.isEmpty()) return false;
    if (QFileInfo(path).suffix().isEmpty()) path += ".obxl";
    return saveTo(path);
}

void MainWindow::controls() {
    QMessageBox::information(this, "Controls",
        "<b>Camera</b><br>Right mouse drag: look &nbsp; W A S D: fly &nbsp; Q / E: down / up<br>"
        "Shift: fly faster &nbsp; Mouse wheel: zoom &nbsp; F: focus selection<br><br>"
        "<b>Tools</b><br>1 Select &nbsp; 2 Move &nbsp; 3 Resize<br>"
        "Drag a part in Select mode to slide it; drag the arrows in Move / Resize mode.<br><br>"
        "<b>Editing</b><br>Ctrl+Z / Y undo, redo &nbsp; Ctrl+C / X / V &nbsp; Ctrl+D duplicate<br>"
        "Ctrl+R rotate &nbsp; Ctrl+T tilt &nbsp; Ctrl+B anchor &nbsp; Ctrl+L lock &nbsp; Del delete<br>"
        "Locked parts cannot be clicked in the 3D view (select them in the Explorer).");
}

void MainWindow::about() {
    QMessageBox::about(this, "About OldBlox Studio",
                       "<b>OldBlox Studio</b><br>A tribute to the 2007-era building tools.<br><br>"
                       "Rendering: G3D Innovation Engine<br>Interface: Qt");
}

void MainWindow::closeEvent(QCloseEvent* e) {
    if (confirmDiscard()) e->accept(); else e->ignore();
}
