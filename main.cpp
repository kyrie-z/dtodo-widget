#include <DApplication>
#include <DMainWindow>
#include <DTitlebar>
#include <DPalette>
#include <DGuiApplicationHelper>
#include <DPaletteHelper>
#include "todowidget.h"

#include <QScreen>
#include <QSettings>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QFile>

#ifdef Q_OS_LINUX
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#endif

DWIDGET_USE_NAMESPACE

class DesktopWidget : public DMainWindow
{
    Q_OBJECT

public:
    explicit DesktopWidget(QWidget *parent = nullptr)
        : DMainWindow(parent)
        , m_dragging(false)
        , m_resizing(false)
        , m_resizeEdge(EdgeNone)
    {
        // 无边框、保持在底部 - 桌面小组件风格
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint | Qt::Tool);

        // 隐藏标题栏并设置固定高度为 0
        titlebar()->setFixedHeight(0);

        // 窗口透明
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);

        // 启用鼠标追踪
        setMouseTracking(true);

        // 使用 DMainWindow 的属性设置窗口样式
        setWindowRadius(12);
        setBorderWidth(0);
        setShadowRadius(20);
        setShadowOffset(QPoint(0, 8));
        setTranslucentBackground(true);
        setEnableSystemResize(false);
        setEnableSystemMove(false);
        setEnableBlurWindow(true);

        // 初始化颜色
        updateBorderColor();
        updateShadowColor();

        // 创建 TodoWidget
        m_todoWidget = new TodoWidget(this);
        m_todoWidget->setMouseTracking(true);
        setCentralWidget(m_todoWidget);

        // 更新背景颜色（需要在 m_todoWidget 创建之后）
        updateBackgroundColor();

        // 设置最小和初始大小
        setMinimumSize(250, 300);
        resize(350, 500);

        // 加载保存的位置和大小
        loadGeometry();

#ifdef Q_OS_LINUX
        // 延迟设置窗口类型，确保窗口已创建
        QTimer::singleShot(100, this, [this]() {
            setUtilityWindowType();
        });
#endif

        // 创建系统托盘
        setupTrayIcon();

        // 监听主题变化，更新边框和阴影颜色
        connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [this]() {
            updateBorderColor();
            updateShadowColor();
            updateBackgroundColor();
        });
    }

protected:
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (!m_dragging && !m_resizing) {
                Edge edge = getResizeEdge(mouseEvent->pos());
                updateCursor(edge);
            }
        }
        return DMainWindow::event(event);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_resizeEdge = getResizeEdge(event->pos());
            if (m_resizeEdge != EdgeNone) {
                m_resizing = true;
                m_resizeStartPos = event->globalPosition().toPoint();
                m_resizeStartGeom = geometry();
            } else {
                m_dragging = true;
                m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
            }
            event->accept();
        }
        DMainWindow::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (m_resizing) {
            QPoint delta = event->globalPosition().toPoint() - m_resizeStartPos;
            QRect newGeom = m_resizeStartGeom;

            int minWidth = minimumWidth();
            int minHeight = minimumHeight();

            if (m_resizeEdge & EdgeLeft) {
                newGeom.setLeft(m_resizeStartGeom.left() + delta.x());
                if (newGeom.width() < minWidth) {
                    newGeom.setLeft(m_resizeStartGeom.right() - minWidth);
                }
            }
            if (m_resizeEdge & EdgeRight) {
                newGeom.setRight(m_resizeStartGeom.right() + delta.x());
                if (newGeom.width() < minWidth) {
                    newGeom.setRight(m_resizeStartGeom.left() + minWidth);
                }
            }
            if (m_resizeEdge & EdgeTop) {
                newGeom.setTop(m_resizeStartGeom.top() + delta.y());
                if (newGeom.height() < minHeight) {
                    newGeom.setTop(m_resizeStartGeom.bottom() - minHeight);
                }
            }
            if (m_resizeEdge & EdgeBottom) {
                newGeom.setBottom(m_resizeStartGeom.bottom() + delta.y());
                if (newGeom.height() < minHeight) {
                    newGeom.setBottom(m_resizeStartGeom.top() + minHeight);
                }
            }

            setGeometry(newGeom);
            event->accept();
        } else if (m_dragging) {
            move(event->globalPosition().toPoint() - m_dragPos);
            event->accept();
        } else {
            // 更新鼠标光标
            Edge edge = getResizeEdge(event->pos());
            updateCursor(edge);
        }
        DMainWindow::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            if (m_resizing) {
                m_resizing = false;
                saveGeometry();
            } else if (m_dragging) {
                m_dragging = false;
                saveGeometry();
            }
            event->accept();
            // 恢复光标
            Edge edge = getResizeEdge(event->pos());
            updateCursor(edge);
        }
        DMainWindow::mouseReleaseEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        setCursor(Qt::ArrowCursor);
        DMainWindow::leaveEvent(event);
    }

    void closeEvent(QCloseEvent *event) override
    {
        saveGeometry();
        DMainWindow::closeEvent(event);
    }

private:
    enum Edge {
        EdgeNone = 0,
        EdgeLeft = 1,
        EdgeRight = 2,
        EdgeTop = 4,
        EdgeBottom = 8
    };

    Edge getResizeEdge(const QPoint &pos)
    {
        const int margin = 8;
        Edge edge = EdgeNone;

        if (pos.x() < margin) edge = Edge(edge | EdgeLeft);
        else if (pos.x() > width() - margin) edge = Edge(edge | EdgeRight);

        if (pos.y() < margin) edge = Edge(edge | EdgeTop);
        else if (pos.y() > height() - margin) edge = Edge(edge | EdgeBottom);

        return edge;
    }

    void updateCursor(Edge edge)
    {
        if (edge == EdgeNone) {
            setCursor(Qt::ArrowCursor);
        } else if (edge == EdgeLeft || edge == EdgeRight) {
            setCursor(Qt::SizeHorCursor);
        } else if (edge == EdgeTop || edge == EdgeBottom) {
            setCursor(Qt::SizeVerCursor);
        } else if (edge == (EdgeLeft | EdgeTop) || edge == (EdgeRight | EdgeBottom)) {
            setCursor(Qt::SizeFDiagCursor);
        } else {
            setCursor(Qt::SizeBDiagCursor);
        }
    }

    // 使用 DTK 主题色更新边框颜色
    void updateBorderColor()
    {
        DPalette pa = DPaletteHelper::instance()->palette(this);
        QColor borderColor = pa.color(DPalette::FrameBorder);
        borderColor.setAlpha(180);
        setBorderColor(borderColor);
    }

    // 使用 DTK 主题色更新阴影颜色
    void updateShadowColor()
    {
        DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType
            ? setShadowColor(QColor(0, 0, 0, 80))
            : setShadowColor(QColor(0, 0, 0, 60));
    }

    // 更新 TodoWidget 的背景色以确保文字可读性
    void updateBackgroundColor()
    {
        if (m_todoWidget) {
            m_todoWidget->updateThemeColors();
        }
    }

    void saveGeometry()
    {
        QSettings settings("dtodo-widget", "geometry");
        settings.setValue("pos", pos());
        settings.setValue("size", size());
    }

    void loadGeometry()
    {
        QSettings settings("dtodo-widget", "geometry");
        QPoint savedPos = settings.value("pos").toPoint();
        QSize savedSize = settings.value("size").toSize();

        QScreen *screen = QGuiApplication::primaryScreen();
        QRect screenRect = screen->availableGeometry();

        if (savedSize.isValid() && savedSize.width() >= minimumWidth() && savedSize.height() >= minimumHeight()) {
            resize(savedSize);
        }

        if (savedPos.x() >= 0 && savedPos.y() >= 0 &&
            savedPos.x() < screenRect.width() - 100 &&
            savedPos.y() < screenRect.height() - 100) {
            move(savedPos);
        } else {
            move(screenRect.right() - width() - 30, screenRect.bottom() - height() - 50);
        }
    }

#ifdef Q_OS_LINUX
    void setUtilityWindowType()
    {
        Display *display = XOpenDisplay(nullptr);
        if (!display) return;

        Window xWinId = static_cast<Window>(winId());

        // 设置窗口类型为 UTILITY，不会被显示桌面最小化，且可以输入
        Atom typeAtom = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
        Atom utilityAtom = XInternAtom(display, "_NET_WM_WINDOW_TYPE_UTILITY", False);
        XChangeProperty(display, xWinId, typeAtom, XA_ATOM, 32,
                        PropModeReplace, (unsigned char *)&utilityAtom, 1);

        XFlush(display);
        XCloseDisplay(display);
    }
#endif

    void setupTrayIcon()
    {
        m_trayIcon = new QSystemTrayIcon(this);
        // 优先使用系统图标路径，fallback 到资源路径
        QString trayIconPath = "/usr/share/icons/hicolor/scalable/apps/dtodo-widget-tray.svg";
        if (QFile::exists(trayIconPath)) {
            m_trayIcon->setIcon(QIcon(trayIconPath));
        } else {
            m_trayIcon->setIcon(QIcon(":/icons/dtodo-widget-tray.svg"));
        }
        m_trayIcon->setToolTip("Todo 小组件");

        QMenu *trayMenu = new QMenu();
        QAction *showAction = trayMenu->addAction("显示窗口");
        QAction *hideAction = trayMenu->addAction("隐藏窗口");
        trayMenu->addSeparator();
        QAction *quitAction = trayMenu->addAction("退出");

        connect(showAction, &QAction::triggered, this, [this]() {
            show();
            raise();
        });
        connect(hideAction, &QAction::triggered, this, &QWidget::hide);
        connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

        m_trayIcon->setContextMenu(trayMenu);

        connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger) {
                if (isVisible()) {
                    hide();
                } else {
                    show();
                    raise();
                }
            }
        });

        m_trayIcon->show();
    }

    TodoWidget *m_todoWidget;
    QSystemTrayIcon *m_trayIcon;
    bool m_dragging;
    bool m_resizing;
    QPoint m_dragPos;
    QPoint m_resizeStartPos;
    QRect m_resizeStartGeom;
    int m_resizeEdge;
};

int main(int argc, char *argv[])
{
    DApplication a(argc, argv);
    a.setProductName("dtodo-widget");
    a.setApplicationName("dtodo-widget");
    a.setApplicationVersion("1.0.0");
    a.setProductIcon(QIcon(":/icons/dtodo-widget.svg"));
    a.setWindowIcon(QIcon(":/icons/dtodo-widget.svg"));

    DesktopWidget w;
    w.setWindowTitle("Todo 小组件");
    w.show();

    return a.exec();
}

#include "main.moc"
