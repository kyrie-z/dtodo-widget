#include <DApplication>
#include <DPlatformWindowHandle>
#include "todowidget.h"

#include <QMainWindow>
#include <QScreen>
#include <QSettings>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QCloseEvent>

DWIDGET_USE_NAMESPACE

class DesktopWidget : public QMainWindow
{
    Q_OBJECT

public:
    explicit DesktopWidget(QWidget *parent = nullptr)
        : QMainWindow(parent)
        , m_dragging(false)
        , m_resizing(false)
        , m_resizeEdge(EdgeNone)
        , m_handle(nullptr)
    {
        // 无边框、保持在底部
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint);
        
        // 窗口透明
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        
        // 启用鼠标追踪
        setMouseTracking(true);
        
        // 启用 dxcb 平台处理
        DPlatformWindowHandle::enableDXcbForWindow(this);
        m_handle = new DPlatformWindowHandle(this, this);
        
        // 设置窗口圆角
        m_handle->setWindowRadius(12);
        
        // 设置边框
        m_handle->setBorderWidth(1);
        m_handle->setBorderColor(QColor(80, 80, 80, 180));
        
        // 设置阴影
        m_handle->setShadowRadius(20);
        m_handle->setShadowOffset(QPoint(0, 8));
        m_handle->setShadowColor(QColor(0, 0, 0, 100));
        
        // 透明背景
        m_handle->setTranslucentBackground(true);
        m_handle->setEnableSystemResize(false);
        m_handle->setEnableSystemMove(false);
        m_handle->setEnableBlurWindow(true);
        
        // 创建 TodoWidget
        m_todoWidget = new TodoWidget(this);
        m_todoWidget->setMouseTracking(true);
        setCentralWidget(m_todoWidget);
        
        // 设置最小和初始大小
        setMinimumSize(250, 300);
        resize(350, 500);
        
        // 加载保存的位置和大小
        loadGeometry();
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
        return QMainWindow::event(event);
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
        QMainWindow::mousePressEvent(event);
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
        QMainWindow::mouseMoveEvent(event);
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
        QMainWindow::mouseReleaseEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        setCursor(Qt::ArrowCursor);
        QMainWindow::leaveEvent(event);
    }

    void closeEvent(QCloseEvent *event) override
    {
        saveGeometry();
        QMainWindow::closeEvent(event);
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

    void saveGeometry()
    {
        QSettings settings("dtk-todo-widget", "geometry");
        settings.setValue("pos", pos());
        settings.setValue("size", size());
    }

    void loadGeometry()
    {
        QSettings settings("dtk-todo-widget", "geometry");
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

    TodoWidget *m_todoWidget;
    DPlatformWindowHandle *m_handle;
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
    a.setProductName("dtk-todo-widget");
    a.setApplicationName("dtk-todo-widget");
    a.setApplicationVersion("1.0.0");
    a.setProductIcon(QIcon::fromTheme("preferences-system"));
    a.setWindowIcon(QIcon::fromTheme("preferences-system"));

    DesktopWidget w;
    w.setWindowTitle("Todo 小组件");
    w.show();
    
    return a.exec();
}

#include "main.moc"
