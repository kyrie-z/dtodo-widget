#include <DApplication>
#include <DMainWindow>
#include "todowidget.h"

#include <QScreen>
#include <QSettings>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QWindow>

DWIDGET_USE_NAMESPACE

class DesktopWidget : public DMainWindow
{
    Q_OBJECT

public:
    explicit DesktopWidget(QWidget *parent = nullptr)
        : DMainWindow(parent)
        , m_dragging(false)
    {
        // 无边框、保持在底部、工具窗口(不显示在任务栏和Alt+Tab)
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint | Qt::Tool);
        
        // 窗口透明
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_X11NetWmWindowTypeDesktop);
        
        // 设置窗口属性
        setWindowRadius(12);
        setBorderWidth(1);
        setBorderColor(QColor(255, 255, 255, 25));
        setShadowRadius(30);
        setShadowOffset(QPoint(0, 10));
        setShadowColor(QColor(0, 0, 0, 80));
        setTranslucentBackground(true);
        setEnableSystemResize(false);
        setEnableSystemMove(false);
        setEnableBlurWindow(true);
        
        // 创建 TodoWidget
        m_todoWidget = new TodoWidget(this);
        setCentralWidget(m_todoWidget);
        
        setFixedSize(350, 500);
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
        DMainWindow::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (m_dragging) {
            move(event->globalPosition().toPoint() - m_dragPos);
            event->accept();
        }
        DMainWindow::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            savePosition();
            event->accept();
        }
        DMainWindow::mouseReleaseEvent(event);
    }

    void closeEvent(QCloseEvent *event) override
    {
        savePosition();
        DMainWindow::closeEvent(event);
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
