/**
 * @file todowidget.h
 * @brief Todo 待办事项列表控件
 *
 * 提供待办事项的显示、添加、编辑、删除、完成状态切换等功能。
 * 使用 DBlurEffectWidget 实现毛玻璃背景效果。
 */

#ifndef TODOWIDGET_H
#define TODOWIDGET_H

#include <DWidget>
#include <DLineEdit>
#include <DPushButton>
#include <DListView>
#include <DLabel>
#include <DBlurEffectWidget>
#include <DMenu>
#include <DStandardItem>
#include <DStyledItemDelegate>
#include <QStandardItemModel>
#include <QStackedWidget>
#include <QSlider>
#include <QPair>

#include "todoitem.h"

DWIDGET_USE_NAMESPACE

/**
 * @class TodoWidget
 * @brief 待办事项列表控件
 *
 * 继承自 DBlurEffectWidget，提供毛玻璃背景效果。
 * 包含输入框、列表视图和统计信息。
 */
class TodoWidget : public DBlurEffectWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父控件
     */
    explicit TodoWidget(QWidget *parent = nullptr);

    /**
     * @brief 更新主题颜色
     *
     * 根据当前系统主题更新控件颜色，包括文字、背景等。
     */
    void updateThemeColors();

protected:
    /**
     * @brief 事件过滤器
     *
     * 用于处理列表项的悬停事件，控制操作按钮的显示/隐藏。
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

private Q_SLOTS:
    /** @brief 添加新的待办事项 */
    void addTodo();

    /** @brief 删除当前选中的待办事项（右键菜单触发） */
    void removeTodo();

    /**
     * @brief 根据 ID 删除待办事项
     * @param todoId 待办事项 ID
     */
    void removeTodoById(int todoId);

    /**
     * @brief 切换待办事项的完成状态
     * @param index 列表项索引
     */
    void toggleCompleted(const QModelIndex &index);

    /**
     * @brief 根据 ID 切换待办事项的完成状态
     * @param todoId 待办事项 ID
     */
    void toggleCompletedById(int todoId);

    /** @brief 回车键按下时添加待办事项 */
    void onReturnPressed();

    /**
     * @brief 显示右键上下文菜单
     * @param pos 鼠标位置
     */
    void showContextMenu(const QPoint &pos);

    /**
     * @brief 列表项数据变化时的处理
     * @param item 变化的列表项
     */
    void onItemChanged(QStandardItem *item);

    /**
     * @brief 双击列表项进入编辑模式
     * @param index 列表项索引
     */
    void onDoubleClicked(const QModelIndex &index);

    /**
     * @brief 显示空白区域右键菜单
     * @param pos 鼠标位置
     */
    void showBlankContextMenu(const QPoint &pos);

    /**
     * @brief 设置模糊半径
     * @param radius 模糊半径值
     */
    void setBlurRadius(int radius);

    /**
     * @brief 设置背景透明度
     * @param alpha 透明度值 (0-255)
     */
    void setMaskAlpha(int alpha);

    /** @brief 关闭窗口 */
    void closeWindow();

private:
    /** @brief 初始化用户界面 */
    void setupUI();

    /** @brief 更新底部统计标签 */
    void updateCountLabel();

    /** @brief 保存待办事项到文件 */
    void saveTodos();

    /** @brief 从文件加载待办事项 */
    void loadTodos();

    /** @brief 更新空状态显示 */
    void updateEmptyState();

    /**
     * @brief 更新列表项外观
     * @param row 行号
     *
     * 根据完成状态设置文字样式（删除线、颜色等）。
     */
    void updateItemAppearance(int row);

    /**
     * @brief 为列表项设置操作按钮
     * @param item 列表项
     * @param todoId 待办事项 ID
     * @param completed 是否已完成
     *
     * 在列表项右侧添加完成和删除按钮。
     */
    void setupItemActions(DStandardItem *item, int todoId, bool completed = false);

    /**
     * @brief 更新操作按钮图标颜色
     * @param row 行号
     * @param completed 是否已完成
     */
    void updateActionIconColor(int row, bool completed);

    /**
     * @brief 创建勾选图标
     * @param completed 是否已完成
     * @return 勾选图标
     */
    QIcon createCheckIcon(bool completed);

    /**
     * @brief 设置列表项操作按钮的可见性
     * @param row 行号
     * @param visible 是否可见
     */
    void setActionsVisible(int row, bool visible);

    /**
     * @brief 初始化空白区域右键菜单
     */
    void setupBlankContextMenu();

    /**
     * @brief 保存模糊效果设置
     */
    void saveBlurSettings();

    /**
     * @brief 加载模糊效果设置
     */
    void loadBlurSettings();

    /**
     * @brief 创建颜色圆点图标
     * @param color 颜色
     * @param size 图标大小
     * @return 颜色圆点图标
     */
    QIcon createColorDotIcon(const QColor &color, int size = 10) const;

    /**
     * @brief 设置待办事项的颜色标记
     * @param colorHex 颜色十六进制值，空字符串表示无颜色
     */
    void setTodoColor(const QString &colorHex);

    DLineEdit *m_inputEdit;             ///< 输入框
    DListView *m_listView;              ///< 列表视图
    QStandardItemModel *m_model;        ///< 列表数据模型
    DLabel *m_countLabel;               ///< 统计标签
    DLabel *m_emptyLabel;               ///< 空状态提示标签
    QStackedWidget *m_stackWidget;      ///< 堆栈窗口（用于切换空态/列表态）
    QWidget *m_listContainer;           ///< 列表容器
    DMenu *m_contextMenu;               ///< 列表项右键菜单
    DMenu *m_blankContextMenu;          ///< 空白区域右键菜单
    QAction *m_toggleAction;            ///< 切换完成状态的菜单项
    QModelIndex m_contextIndex;         ///< 右键菜单触发时的索引
    QList<TodoItem> m_todos;            ///< 待办事项数据列表
    int m_nextId;                       ///< 下一个待办事项 ID
    bool m_isEditing;                   ///< 是否正在编辑
    int m_hoveredRow;                   ///< 当前悬停的行号
    int m_blurRadius;                   ///< 模糊半径
    int m_maskAlpha;                    ///< 背景透明度
};

#endif // TODOWIDGET_H
