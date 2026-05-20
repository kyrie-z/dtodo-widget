/**
 * @file todowidget.cpp
 * @brief TodoWidget 实现
 */

#include "todowidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QTimer>
#include <QMouseEvent>
#include <QSettings>
#include <DPalette>
#include <DGuiApplicationHelper>
#include <DPaletteHelper>
#include <DStyle>
#include <DMainWindow>

DWIDGET_USE_NAMESPACE

/**
 * @brief 构造函数实现
 *
 * 初始化毛玻璃效果、UI 界面、加载已保存的数据。
 */
TodoWidget::TodoWidget(QWidget *parent)
    : DBlurEffectWidget(parent)
    , m_nextId(1)
    , m_contextMenu(nullptr)
    , m_blankContextMenu(nullptr)
    , m_isEditing(false)
    , m_hoveredRow(-1)
    , m_blurRadius(40)
    , m_maskAlpha(180)
{
    // 加载模糊效果设置
    loadBlurSettings();

    // 设置模糊特效 - 符合 DTK 设计规范的毛玻璃效果
    setMode(DBlurEffectWidget::GaussianBlur);
    setBlendMode(DBlurEffectWidget::BehindWindowBlend);
    setRadius(m_blurRadius);            // 模糊半径
    setBlurRectXRadius(12);             // 圆角 X 半径
    setBlurRectYRadius(12);             // 圆角 Y 半径

    // 使用 AutoColor 自动跟随主题
    // 浅色主题白色背景，深色主题深色背景
    setMaskColor(AutoColor);

    // 设置背景不透明度，增强模糊和透明效果
    setMaskAlpha(m_maskAlpha);

    // 初始化 UI
    setupUI();

    // 初始化空白区域右键菜单
    setupBlankContextMenu();

    // 加载已保存的待办事项
    loadTodos();

    // 延迟更新主题颜色，确保 widget 完全初始化
    QTimer::singleShot(0, this, &TodoWidget::updateThemeColors);
}

/**
 * @brief 初始化用户界面
 *
 * 创建输入区域、列表视图、空状态提示、底部统计和右键菜单。
 */
void TodoWidget::setupUI()
{
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // ========== 输入区域 ==========
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(10);

    m_inputEdit = new DLineEdit(this);
    m_inputEdit->setPlaceholderText("添加新的待办事项...");
    m_inputEdit->setClearButtonEnabled(true);

    DPushButton *addBtn = new DPushButton("添加", this);
    addBtn->setFixedSize(64, 36);

    inputLayout->addWidget(m_inputEdit);
    inputLayout->addWidget(addBtn);

    mainLayout->addLayout(inputLayout);

    // ========== 列表区域 ==========
    // 使用 QStackedWidget 实现空态切换
    m_stackWidget = new QStackedWidget(this);

    // 列表容器
    m_listContainer = new QWidget(m_stackWidget);
    QVBoxLayout *listLayout = new QVBoxLayout(m_listContainer);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(0);

    // 列表视图
    m_listView = new DListView(m_listContainer);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setBackgroundType(DStyledItemDelegate::RoundedBackground);
    m_listView->setItemSpacing(6);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setViewportMargins(0, 0, 0, 0);
    m_listView->setMouseTracking(true);     // 启用鼠标追踪以检测悬停

    // 启用拖拽排序（参考 DTK 示例）
    m_listView->setDragDropMode(QListView::InternalMove);

    // 数据模型
    m_model = new QStandardItemModel(this);
    // 设置 item 原型为 DStandardItem，确保拖拽时数据保留
    m_model->setItemPrototype(new DStandardItem());
    m_listView->setModel(m_model);

    // 安装事件过滤器以处理悬停事件
    m_listView->viewport()->installEventFilter(this);

    listLayout->addWidget(m_listView);

    // ========== 空态提示 ==========
    QWidget *emptyWidget = new QWidget(m_stackWidget);
    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyWidget);
    emptyLayout->setAlignment(Qt::AlignCenter);

    m_emptyLabel = new DLabel(emptyWidget);
    m_emptyLabel->setText("暂无待办事项\n输入内容后按回车添加");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    DPalette pa = DPaletteHelper::instance()->palette(m_emptyLabel);
    pa.setColor(DPalette::WindowText, pa.color(DPalette::PlaceholderText));
    DPaletteHelper::instance()->setPalette(m_emptyLabel, pa);
    m_emptyLabel->setWordWrap(true);

    emptyLayout->addWidget(m_emptyLabel);

    m_stackWidget->addWidget(m_listContainer);
    m_stackWidget->addWidget(emptyWidget);

    mainLayout->addWidget(m_stackWidget, 1);

    // ========== 底部统计 ==========
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    m_countLabel = new DLabel("共 0 项", this);
    bottomLayout->addWidget(m_countLabel);
    bottomLayout->addStretch();

    mainLayout->addLayout(bottomLayout);

    // ========== 右键菜单 ==========
    m_contextMenu = new DMenu(this);
    QAction *editAction = m_contextMenu->addAction("编辑");
    QAction *toggleAction = m_contextMenu->addAction("切换完成状态");
    QAction *deleteAction = m_contextMenu->addAction("删除");

    // ========== 连接信号 ==========
    connect(addBtn, &DPushButton::clicked, this, &TodoWidget::addTodo);
    connect(m_inputEdit, &DLineEdit::returnPressed, this, &TodoWidget::onReturnPressed);
    connect(m_listView, &DListView::doubleClicked, this, &TodoWidget::onDoubleClicked);
    connect(m_listView, &DListView::customContextMenuRequested, this, &TodoWidget::showContextMenu);
    connect(m_model, &QStandardItemModel::itemChanged, this, &TodoWidget::onItemChanged);

    // 新插入行后更新拖拽属性
    connect(m_model, &QStandardItemModel::rowsInserted, this, [this]() {
        for (int i = 0; i < m_model->rowCount(); ++i) {
            QStandardItem *item = m_model->item(i);
            if (item) {
                item->setDragEnabled(true);
                item->setDropEnabled(false);
            }
        }
    });

    // 拖拽完成后保存数据
    connect(m_model, &QStandardItemModel::rowsMoved, this, [this]() {
        saveTodos();
    });

    // 右键菜单动作
    connect(editAction, &QAction::triggered, this, [this]() {
        if (m_contextIndex.isValid()) {
            m_listView->edit(m_contextIndex);
        }
    });
    connect(toggleAction, &QAction::triggered, this, [this]() {
        if (m_contextIndex.isValid()) {
            toggleCompleted(m_contextIndex);
        }
    });
    connect(deleteAction, &QAction::triggered, this, &TodoWidget::removeTodo);

    // 监听主题变化
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, &TodoWidget::updateThemeColors);

    // 为其他控件安装事件过滤器以处理右键菜单
    m_countLabel->installEventFilter(this);
    m_stackWidget->installEventFilter(this);

    updateCountLabel();
    updateEmptyState();
}

/**
 * @brief 事件过滤器 - 处理列表项悬停事件和右键菜单事件
 *
 * 当鼠标进入某个列表项时，显示该行的操作按钮；
 * 当鼠标离开时，隐藏操作按钮。
 * 处理空白区域的右键菜单事件（不影响列表项的右键菜单）。
 */
bool TodoWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_listView->viewport()) {
        // 鼠标移动事件 - 检测悬停的列表项
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            QModelIndex index = m_listView->indexAt(mouseEvent->pos());
            int currentRow = index.isValid() ? index.row() : -1;

            // 如果悬停的行发生变化
            if (currentRow != m_hoveredRow) {
                // 隐藏之前悬停行的操作按钮
                if (m_hoveredRow >= 0 && m_hoveredRow < m_model->rowCount()) {
                    setActionsVisible(m_hoveredRow, false);
                }
                // 显示当前悬停行的操作按钮
                if (currentRow >= 0) {
                    setActionsVisible(currentRow, true);
                }
                m_hoveredRow = currentRow;
            }
        }
        // 鼠标离开事件 - 隐藏所有操作按钮
        else if (event->type() == QEvent::Leave) {
            if (m_hoveredRow >= 0 && m_hoveredRow < m_model->rowCount()) {
                setActionsVisible(m_hoveredRow, false);
            }
            m_hoveredRow = -1;
        }
    }

    // 处理其他空白区域的右键菜单事件（不影响输入框和列表项）
    if (event->type() == QEvent::ContextMenu) {
        // 检查是否来自输入框或其内部控件
        if (watched == m_inputEdit || watched == m_inputEdit->lineEdit()) {
            return DBlurEffectWidget::eventFilter(watched, event);  // 不处理，保持默认行为
        }

        // 检查是否来自列表视图的viewport（列表项菜单已在showContextMenu中处理）
        if (watched == m_listView->viewport()) {
            return DBlurEffectWidget::eventFilter(watched, event);  // 不处理，保持默认行为
        }

        // 其他区域显示设置菜单
        QContextMenuEvent *contextEvent = static_cast<QContextMenuEvent *>(event);
        m_blankContextMenu->exec(contextEvent->globalPos());
        return true;  // 事件已处理
    }

    return DBlurEffectWidget::eventFilter(watched, event);
}

/**
 * @brief 添加新的待办事项
 */
void TodoWidget::addTodo()
{
    QString text = m_inputEdit->text().trimmed();
    if (text.isEmpty()) {
        return;
    }

    // 创建待办事项数据
    TodoItem item(text);
    item.setId(m_nextId++);
    m_todos.append(item);

    // 创建列表项（使用 DStandardItem）
    DStandardItem *modelItem = new DStandardItem(text);
    modelItem->setEditable(true);
    modelItem->setDragEnabled(true);
    modelItem->setDropEnabled(false);
    modelItem->setData(item.id(), Qt::UserRole);

    // 设置操作按钮（初始隐藏）
    setupItemActions(modelItem, item.id());

    m_model->appendRow(modelItem);

    // 应用外观样式
    updateItemAppearance(m_model->rowCount() - 1);

    m_inputEdit->clear();
    updateCountLabel();
    updateEmptyState();
    saveTodos();
}

/**
 * @brief 为列表项设置操作按钮
 *
 * 在列表项右侧添加完成和删除按钮，初始状态为隐藏。
 */
void TodoWidget::setupItemActions(DStandardItem *item, int todoId)
{
    // 完成按钮
    DViewItemAction *completeAction = new DViewItemAction(Qt::AlignVCenter, QSize(20, 20), QSize(20, 20), true);
    completeAction->setIcon(QIcon::fromTheme("dialog-ok"));
    completeAction->setToolTip("标记完成");
    completeAction->setVisible(false);  // 初始隐藏

    // 删除按钮
    DViewItemAction *deleteAction = new DViewItemAction(Qt::AlignVCenter, QSize(20, 20), QSize(20, 20), true);
    deleteAction->setIcon(QIcon::fromTheme("edit-delete"));
    deleteAction->setToolTip("删除");
    deleteAction->setVisible(false);    // 初始隐藏

    // 连接信号
    connect(completeAction, &DViewItemAction::triggered, this, [this, todoId]() {
        toggleCompletedById(todoId);
    });

    connect(deleteAction, &DViewItemAction::triggered, this, [this, todoId]() {
        removeTodoById(todoId);
    });

    // 设置到列表项右侧
    item->setActionList(Qt::Edge::RightEdge, {completeAction, deleteAction});
}

/**
 * @brief 设置列表项操作按钮的可见性
 */
void TodoWidget::setActionsVisible(int row, bool visible)
{
    if (row < 0 || row >= m_model->rowCount()) return;

    DStandardItem *item = dynamic_cast<DStandardItem *>(m_model->item(row));
    if (!item) return;

    DViewItemActionList actions = item->actionList(Qt::RightEdge);
    for (DViewItemAction *action : actions) {
        action->setVisible(visible);
    }

    // 刷新显示
    m_listView->update(m_model->index(row, 0));
}

/**
 * @brief 根据 ID 切换待办事项的完成状态
 */
void TodoWidget::toggleCompletedById(int todoId)
{
    for (int i = 0; i < m_todos.size(); ++i) {
        if (m_todos[i].id() == todoId) {
            bool completed = !m_todos[i].completed();
            m_todos[i].setCompleted(completed);

            // 更新外观
            for (int row = 0; row < m_model->rowCount(); ++row) {
                QStandardItem *item = m_model->item(row);
                if (item && item->data(Qt::UserRole).toInt() == todoId) {
                    updateItemAppearance(row);
                    break;
                }
            }
            break;
        }
    }
    updateCountLabel();
    saveTodos();
}

/**
 * @brief 根据 ID 删除待办事项
 */
void TodoWidget::removeTodoById(int todoId)
{
    // 从数据列表中移除
    for (int i = 0; i < m_todos.size(); ++i) {
        if (m_todos[i].id() == todoId) {
            m_todos.removeAt(i);
            break;
        }
    }

    // 从模型中移除
    for (int row = 0; row < m_model->rowCount(); ++row) {
        QStandardItem *item = m_model->item(row);
        if (item && item->data(Qt::UserRole).toInt() == todoId) {
            m_model->removeRow(row);
            break;
        }
    }

    updateCountLabel();
    updateEmptyState();
    saveTodos();
}

/**
 * @brief 删除当前选中的待办事项（右键菜单触发）
 */
void TodoWidget::removeTodo()
{
    if (!m_contextIndex.isValid()) return;

    QStandardItem *item = m_model->item(m_contextIndex.row());
    if (!item) return;

    int todoId = item->data(Qt::UserRole).toInt();
    removeTodoById(todoId);
}

/**
 * @brief 切换待办事项的完成状态
 */
void TodoWidget::toggleCompleted(const QModelIndex &index)
{
    if (!index.isValid()) return;

    QStandardItem *item = m_model->item(index.row());
    if (!item) return;

    int todoId = item->data(Qt::UserRole).toInt();
    toggleCompletedById(todoId);
}

/**
 * @brief 回车键按下时添加待办事项
 */
void TodoWidget::onReturnPressed()
{
    addTodo();
}

/**
 * @brief 显示右键上下文菜单
 * @param pos 鼠标位置（相对于列表视图）
 *
 * 点击列表项时显示列表项菜单，点击空白区域时显示设置菜单。
 */
void TodoWidget::showContextMenu(const QPoint &pos)
{
    QModelIndex index = m_listView->indexAt(pos);
    if (index.isValid()) {
        // 点击的是列表项，显示列表项菜单
        m_contextIndex = index;
        m_contextMenu->exec(m_listView->viewport()->mapToGlobal(pos));
    } else {
        // 点击的是列表区域的空白部分，显示设置菜单
        m_blankContextMenu->exec(m_listView->viewport()->mapToGlobal(pos));
    }
}

/**
 * @brief 更新底部统计标签
 */
void TodoWidget::updateCountLabel()
{
    int total = m_todos.size();
    int completed = 0;
    for (const auto &item : m_todos) {
        if (item.completed()) completed++;
    }
    m_countLabel->setText(QString("共 %1 项 | 已完成 %2 项").arg(total).arg(completed));
}

/**
 * @brief 更新空状态显示
 */
void TodoWidget::updateEmptyState()
{
    if (m_model->rowCount() == 0) {
        m_stackWidget->setCurrentWidget(m_emptyLabel->parentWidget());
    } else {
        m_stackWidget->setCurrentWidget(m_listContainer);
    }
}

/**
 * @brief 保存待办事项到文件
 *
 * 先根据模型顺序同步数据，再写入 JSON 文件。
 */
void TodoWidget::saveTodos()
{
    // 根据 model 当前顺序同步 m_todos
    QList<TodoItem> newTodos;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        QStandardItem *modelItem = m_model->item(i);
        if (modelItem) {
            int todoId = modelItem->data(Qt::UserRole).toInt();
            for (const auto &todo : m_todos) {
                if (todo.id() == todoId) {
                    newTodos.append(todo);
                    break;
                }
            }
        }
    }
    m_todos = newTodos;

    // 写入文件
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile file(dataPath + "/todos.json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonArray array;
        for (const auto &item : m_todos) {
            QJsonObject obj;
            obj["id"] = item.id();
            obj["text"] = item.text();
            obj["completed"] = item.completed();
            obj["createdAt"] = item.createdAt().toString(Qt::ISODate);
            array.append(obj);
        }
        QJsonDocument doc(array);
        file.write(doc.toJson());
        file.close();
    }
}

/**
 * @brief 从文件加载待办事项
 */
void TodoWidget::loadTodos()
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile file(dataPath + "/todos.json");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray jsonData = file.readAll();
        file.close();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "JSON parse error:" << parseError.errorString();
            updateCountLabel();
            updateEmptyState();
            return;
        }

        if (!doc.isArray()) {
            qWarning() << "Invalid JSON format: expected array";
            updateCountLabel();
            updateEmptyState();
            return;
        }

        QJsonArray array = doc.array();

        for (const auto &value : array) {
            QJsonObject obj = value.toObject();
            TodoItem item(obj["text"].toString());
            item.setId(obj["id"].toInt());
            item.setCompleted(obj["completed"].toBool());
            item.setCreatedAt(QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate));
            m_todos.append(item);

            if (item.id() >= m_nextId) {
                m_nextId = item.id() + 1;
            }

            // 创建列表项
            DStandardItem *modelItem = new DStandardItem(item.text());
            modelItem->setEditable(true);
            modelItem->setDragEnabled(true);
            modelItem->setDropEnabled(false);
            modelItem->setData(item.id(), Qt::UserRole);

            // 设置操作按钮（初始隐藏）
            setupItemActions(modelItem, item.id());

            m_model->appendRow(modelItem);

            // 应用外观样式
            updateItemAppearance(m_model->rowCount() - 1);
        }
    }
    updateCountLabel();
    updateEmptyState();
}

/**
 * @brief 更新主题颜色
 */
void TodoWidget::updateThemeColors()
{
    if (m_inputEdit) {
        DPalette inputPa = DPaletteHelper::instance()->palette(m_inputEdit);
        m_inputEdit->setPalette(inputPa);
    }

    if (m_emptyLabel) {
        DPalette emptyPa = DPaletteHelper::instance()->palette(m_emptyLabel);
        emptyPa.setColor(DPalette::WindowText, emptyPa.color(DPalette::PlaceholderText));
        DPaletteHelper::instance()->setPalette(m_emptyLabel, emptyPa);
    }

    if (m_model && m_listView) {
        for (int i = 0; i < m_model->rowCount(); ++i) {
            updateItemAppearance(i);
        }
    }
}

/**
 * @brief 更新列表项外观
 *
 * 根据完成状态设置文字样式：已完成项显示删除线并使用次要文字颜色。
 */
void TodoWidget::updateItemAppearance(int row)
{
    if (row < 0 || row >= m_model->rowCount()) return;

    QStandardItem *item = m_model->item(row);
    if (!item) return;

    int todoId = item->data(Qt::UserRole).toInt();

    // 查找对应的待办事项获取完成状态
    bool completed = false;
    for (const auto &todo : m_todos) {
        if (todo.id() == todoId) {
            completed = todo.completed();
            break;
        }
    }

    // 设置字体样式（已完成项显示删除线）
    QFont font = item->font();
    font.setStrikeOut(completed);
    item->setFont(font);

    // 设置前景色（已完成项使用次要文字颜色）
    DPalette pa = DPaletteHelper::instance()->palette(m_listView);
    if (completed) {
        item->setForeground(pa.color(DPalette::PlaceholderText));
    } else {
        item->setForeground(pa.color(DPalette::Text));
    }

    // 设置背景色
    QColor bgColor = pa.color(DPalette::Base);
    bgColor.setAlpha(200);
    item->setBackground(bgColor);
}

/**
 * @brief 列表项数据变化时的处理
 *
 * 处理文本编辑后的保存。
 */
void TodoWidget::onItemChanged(QStandardItem *item)
{
    if (!item || m_isEditing) return;

    int todoId = item->data(Qt::UserRole).toInt();

    for (int i = 0; i < m_todos.size(); ++i) {
        if (m_todos[i].id() == todoId) {
            // 检查是否是文本编辑
            if (item->text() != m_todos[i].text()) {
                m_todos[i].setText(item->text());
                saveTodos();
            }
            break;
        }
    }
}

/**
 * @brief 双击列表项进入编辑模式
 */
void TodoWidget::onDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    m_isEditing = true;
    m_listView->edit(index);
    QTimer::singleShot(100, this, [this]() { m_isEditing = false; });
}

/**
 * @brief 初始化空白区域右键菜单
 *
 * 创建包含模糊效果设置和关闭选项的右键菜单。
 */
void TodoWidget::setupBlankContextMenu()
{
    m_blankContextMenu = new DMenu(this);

    // ========== 模糊效果子菜单 ==========
    DMenu *blurMenu = m_blankContextMenu->addMenu("模糊效果");

    // 模糊半径子菜单
    DMenu *radiusMenu = blurMenu->addMenu("模糊半径");
    QAction *radiusLow = radiusMenu->addAction("低 (20)");
    QAction *radiusMedium = radiusMenu->addAction("中 (40)");
    QAction *radiusHigh = radiusMenu->addAction("高 (60)");

    // 设置当前选中状态
    radiusMedium->setCheckable(true);
    radiusLow->setCheckable(true);
    radiusHigh->setCheckable(true);

    // 根据当前设置勾选对应选项
    if (m_blurRadius <= 20) {
        radiusLow->setChecked(true);
    } else if (m_blurRadius >= 60) {
        radiusHigh->setChecked(true);
    } else {
        radiusMedium->setChecked(true);
    }

    // 透明度子菜单
    DMenu *alphaMenu = blurMenu->addMenu("背景透明度");
    QAction *alphaLow = alphaMenu->addAction("低 (较透明, 120)");
    QAction *alphaMedium = alphaMenu->addAction("中 (默认, 180)");
    QAction *alphaHigh = alphaMenu->addAction("高 (较不透明, 220)");

    alphaLow->setCheckable(true);
    alphaMedium->setCheckable(true);
    alphaHigh->setCheckable(true);

    // 根据当前设置勾选对应选项
    if (m_maskAlpha <= 120) {
        alphaLow->setChecked(true);
    } else if (m_maskAlpha >= 220) {
        alphaHigh->setChecked(true);
    } else {
        alphaMedium->setChecked(true);
    }

    // 连接模糊半径信号
    connect(radiusLow, &QAction::triggered, this, [this, radiusLow, radiusMedium, radiusHigh]() {
        setBlurRadius(20);
        radiusLow->setChecked(true);
        radiusMedium->setChecked(false);
        radiusHigh->setChecked(false);
    });
    connect(radiusMedium, &QAction::triggered, this, [this, radiusLow, radiusMedium, radiusHigh]() {
        setBlurRadius(40);
        radiusLow->setChecked(false);
        radiusMedium->setChecked(true);
        radiusHigh->setChecked(false);
    });
    connect(radiusHigh, &QAction::triggered, this, [this, radiusLow, radiusMedium, radiusHigh]() {
        setBlurRadius(60);
        radiusLow->setChecked(false);
        radiusMedium->setChecked(false);
        radiusHigh->setChecked(true);
    });

    // 连接透明度信号
    connect(alphaLow, &QAction::triggered, this, [this, alphaLow, alphaMedium, alphaHigh]() {
        setMaskAlpha(120);
        alphaLow->setChecked(true);
        alphaMedium->setChecked(false);
        alphaHigh->setChecked(false);
    });
    connect(alphaMedium, &QAction::triggered, this, [this, alphaLow, alphaMedium, alphaHigh]() {
        setMaskAlpha(180);
        alphaLow->setChecked(false);
        alphaMedium->setChecked(true);
        alphaHigh->setChecked(false);
    });
    connect(alphaHigh, &QAction::triggered, this, [this, alphaLow, alphaMedium, alphaHigh]() {
        setMaskAlpha(220);
        alphaLow->setChecked(false);
        alphaMedium->setChecked(false);
        alphaHigh->setChecked(true);
    });

    // 分隔线
    m_blankContextMenu->addSeparator();

    // ========== 关闭选项 ==========
    QAction *closeAction = m_blankContextMenu->addAction("关闭");
    connect(closeAction, &QAction::triggered, this, &TodoWidget::closeWindow);
}

/**
 * @brief 显示空白区域右键菜单
 * @param pos 鼠标位置
 *
 * 当鼠标点击空白区域时显示设置菜单。
 */
void TodoWidget::showBlankContextMenu(const QPoint &pos)
{
    // 检查是否点击在列表项上，如果是则不显示空白菜单
    QModelIndex index = m_listView->indexAt(mapFromGlobal(mapToGlobal(pos)));
    if (index.isValid()) {
        return;
    }

    m_blankContextMenu->exec(mapToGlobal(pos));
}

/**
 * @brief 设置模糊半径
 * @param radius 模糊半径值
 *
 * 更新模糊效果并保存设置。
 */
void TodoWidget::setBlurRadius(int radius)
{
    m_blurRadius = radius;
    setRadius(radius);
    saveBlurSettings();
}

/**
 * @brief 设置背景透明度
 * @param alpha 透明度值 (0-255)
 *
 * 更新背景透明度并保存设置。
 */
void TodoWidget::setMaskAlpha(int alpha)
{
    m_maskAlpha = alpha;
    DBlurEffectWidget::setMaskAlpha(alpha);
    saveBlurSettings();
}

/**
 * @brief 关闭窗口
 *
 * 关闭父窗口（DesktopWidget）。
 */
void TodoWidget::closeWindow()
{
    // 查找父窗口并关闭
    QWidget *parent = this->parentWidget();
    if (parent) {
        parent->close();
    }
}

/**
 * @brief 保存模糊效果设置
 *
 * 将模糊半径和透明度设置保存到配置文件。
 */
void TodoWidget::saveBlurSettings()
{
    QSettings settings("dtodo-widget", "blur");
    settings.setValue("radius", m_blurRadius);
    settings.setValue("alpha", m_maskAlpha);
}

/**
 * @brief 加载模糊效果设置
 *
 * 从配置文件加载模糊效果设置。
 */
void TodoWidget::loadBlurSettings()
{
    QSettings settings("dtodo-widget", "blur");
    m_blurRadius = settings.value("radius", 40).toInt();
    m_maskAlpha = settings.value("alpha", 180).toInt();

    // 限制范围
    m_blurRadius = qBound(10, m_blurRadius, 100);
    m_maskAlpha = qBound(50, m_maskAlpha, 255);
}
