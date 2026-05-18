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
        , m_handle(nullptr)
    {
        // 无边框、保持在底部
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint);
        
        // 窗口透明
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        
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
        setCentralWidget(m_todoWidget);
        
        // 设置固定大小
        setFixedSize(350, 500);
        
        // 加载保存的位置
        loadPosition();
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
        }
        QMainWindow::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (m_dragging) {
            move(event->globalPosition().toPoint() - m_dragPos);
            event->accept();
        }
        QMainWindow::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            savePosition();
            event->accept();
        }
        QMainWindow::mouseReleaseEvent(event);
    }

    void closeEvent(QCloseEvent *event) override
    {
        savePosition();
        QMainWindow::closeEvent(event);
    }

private:
    void savePosition()
    {
        QSettings settings("dtk-todo-widget", "position");
        settings.setValue("pos", pos());
    }

    void loadPosition()
    {
        QSettings settings("dtk-todo-widget", "position");
        QPoint savedPos = settings.value("pos").toPoint();
        
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect screenRect = screen->availableGeometry();
        
        if (savedPos.x() >= 0 && savedPos.y() >= 0 &&
            savedPos.x() < screenRect.width() - 100 &&
            savedPos.y() < screenRect.height() - 100) {
            move(savedPos);
        } else {
            move(screenRect.right() - 380, screenRect.bottom() - 550);
        }
    }

    TodoWidget *m_todoWidget;
    DPlatformWindowHandle *m_handle;
    bool m_dragging;
    QPoint m_dragPos;
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
