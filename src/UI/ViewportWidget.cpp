#include "ViewportWidget.h"
#include <QPainter>
#include <QResizeEvent>

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#endif

ViewportWidget::ViewportWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_NativeWindow, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setMinimumSize(320, 240);
    setFocusPolicy(Qt::StrongFocus);
    m_focusTimer.setInterval(30);
    connect(&m_focusTimer, &QTimer::timeout, this, &ViewportWidget::pollFocus);
}

bool ViewportWidget::attachNativeWindow(void* nativeHandle) {
#ifdef _WIN32
    HWND child = static_cast<HWND>(nativeHandle);
    HWND parent = reinterpret_cast<HWND>(winId());
    if (!child || !parent) { return false; }

    LONG_PTR style = GetWindowLongPtr(child, GWL_STYLE);
    style &= ~static_cast<LONG_PTR>(WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX |
                                    WS_MAXIMIZEBOX | WS_BORDER | WS_DLGFRAME);
    style |= WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
    SetWindowLongPtr(child, GWL_STYLE, style);

    LONG_PTR ex = GetWindowLongPtr(child, GWL_EXSTYLE);
    ex &= ~static_cast<LONG_PTR>(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_TOOLWINDOW);
    SetWindowLongPtr(child, GWL_EXSTYLE, ex);

    SetParent(child, parent);
    m_child = child;
    layoutChild();
    ShowWindow(child, SW_SHOW);
    m_focusTimer.start();
    update();
    return true;
#else
    Q_UNUSED(nativeHandle);
    return false;
#endif
}

void ViewportWidget::layoutChild() {
#ifdef _WIN32
    if (!m_child) { return; }
    const qreal dpr = devicePixelRatioF();
    const int w = qMax(1, static_cast<int>(width() * dpr));
    const int h = qMax(1, static_cast<int>(height() * dpr));
    SetWindowPos(static_cast<HWND>(m_child), nullptr, 0, 0, w, h,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
#endif
}

void ViewportWidget::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    layoutChild();
}

// A child HWND does not take keyboard focus by itself when clicked, so give it focus on mouse-down.
void ViewportWidget::pollFocus() {
#ifdef _WIN32
    if (!m_child) { return; }
    HWND child = static_cast<HWND>(m_child);
    const bool down = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) || (GetAsyncKeyState(VK_RBUTTON) & 0x8000) ||
                      (GetAsyncKeyState(VK_MBUTTON) & 0x8000);
    if (!down || GetFocus() == child) { return; }
    POINT pt;
    if (GetCursorPos(&pt) && WindowFromPoint(pt) == child) { SetFocus(child); }
#endif
}

void ViewportWidget::paintEvent(QPaintEvent*) {
    if (m_child) { return; }
    QPainter p(this);
    QLinearGradient g(0, 0, 0, height());
    g.setColorAt(0.0, QColor(96, 150, 214));
    g.setColorAt(1.0, QColor(178, 210, 240));
    p.fillRect(rect(), g);
    p.setPen(QColor(255, 255, 255, 200));
    p.drawText(rect(), Qt::AlignCenter, tr("3D viewport"));
}
