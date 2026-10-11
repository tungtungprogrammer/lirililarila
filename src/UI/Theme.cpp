#include "Theme.h"
#include <QApplication>
#include <QFont>
#include <QPalette>
#include <QStyleFactory>

void applyClassicTheme(QApplication& app) {
    if (QStyle* s = QStyleFactory::create("Windows")) { app.setStyle(s); }

    const QColor face(236, 233, 216);       // XP Luna "button face"
    const QColor highlight(49, 106, 197);   // XP selection blue

    QPalette p;
    p.setColor(QPalette::Window, face);
    p.setColor(QPalette::WindowText, Qt::black);
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::AlternateBase, QColor(245, 244, 234));
    p.setColor(QPalette::Text, Qt::black);
    p.setColor(QPalette::Button, face);
    p.setColor(QPalette::ButtonText, Qt::black);
    p.setColor(QPalette::Light, Qt::white);
    p.setColor(QPalette::Midlight, QColor(241, 239, 226));
    p.setColor(QPalette::Mid, QColor(172, 168, 153));
    p.setColor(QPalette::Dark, QColor(113, 111, 100));
    p.setColor(QPalette::Shadow, QColor(64, 64, 64));
    p.setColor(QPalette::Highlight, highlight);
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::ToolTipBase, QColor(255, 255, 225));
    p.setColor(QPalette::ToolTipText, Qt::black);
    p.setColor(QPalette::PlaceholderText, QColor(120, 120, 120));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(172, 168, 153));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(172, 168, 153));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(172, 168, 153));
    app.setPalette(p);

    QFont f("Tahoma", 8);
    f.setStyleHint(QFont::SansSerif);
    app.setFont(f);

    app.setStyleSheet(
        "QDockWidget { font-weight: bold; }"
        "QDockWidget::title { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #ffffff, stop:1 #d4d0c0);"
        "                     border: 1px solid #aca899; padding: 2px 6px; text-align: left; }"
        "QToolBar { border: none; spacing: 1px; padding: 1px; }"
        "QToolButton { border: 1px solid transparent; padding: 2px; }"
        "QToolButton:hover { border: 1px solid #316ac5; background: #c1d2ee; }"
        "QToolButton:checked, QToolButton:pressed { border: 1px solid #316ac5; background: #98b8e8; }"
        "QTreeWidget, QListWidget, QPlainTextEdit { border: 1px solid #7f9db9; background: white; }"
        "QStatusBar::item { border: none; }"
    );
}
