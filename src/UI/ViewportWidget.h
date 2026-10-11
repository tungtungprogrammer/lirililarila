#pragma once
#include <QTimer>
#include <QWidget>

// Hosts the 3D viewport. On Windows the renderer's native window is re-parented into this widget;
// elsewhere (and before the renderer attaches) it shows a flat sky-coloured placeholder.
class ViewportWidget : public QWidget {
    Q_OBJECT
public:
    explicit ViewportWidget(QWidget* parent = nullptr);

    // Re-parent a native window (HWND on Windows) into this widget. Returns false if unsupported.
    bool attachNativeWindow(void* nativeHandle);
    bool isEmbedded() const { return m_child != nullptr; }

    QSize sizeHint() const override { return QSize(900, 600); }

protected:
    void resizeEvent(QResizeEvent* e) override;
    void paintEvent(QPaintEvent* e) override;

private:
    void layoutChild();
    void pollFocus();

    void*   m_child = nullptr;
    QTimer  m_focusTimer;
};
