#include "todowidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QGraphicsDropShadowEffect>
#include <DPalette>
#include <DGuiApplicationHelper>
#include <DPaletteHelper>

DWIDGET_USE_NAMESPACE

TodoWidget::TodoWidget(QWidget *parent)
    : DBlurEffectWidget(parent)
    , m_nextId(1)
    , m_contextMenu(nullptr)
{
    // 设置模糊特效 - 符合 DTK 设计规范的毛玻璃效果
    setMode(DBlurEffectWidget::GaussianBlur);
    setBlendMode(DBlurEffectWidget::BehindWindowBlend);
    setRadius(30);
    setBlurRectXRadius(12);
    setBlurRectYRadius(12);

    // 使用 AutoColor 自动跟随主题
    // 浅色主题白色背景，深色主题深色背景
    setMaskColor(AutoColor);

    setupUI();
    loadTodos();
}

void TodoWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // 输入区域
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

    // 使用 QStackedWidget 实现空态切换
    m_stackWidget = new QStackedWidget(this);

    // 列表容器
    m_listContainer = new QWidget(m_stackWidget);
    QVBoxLayout *listLayout = new QVBoxLayout(m_listContainer);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(0);

    m_listView = new DListView(m_listContainer);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setBackgroundType(DStyledItemDelegate::RoundedBackground);
    m_listView->setItemSpacing(6);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setViewportMargins(0, 0, 0, 0);

    // 启用 hover 效果
    m_listView->setMouseTracking(true);

    m_model = new QStandardItemModel(this);
    m_listView->setModel(m_model);

    listLayout->addWidget(m_listView);

    // 空态提示
    QWidget *emptyWidget = new QWidget(m_stackWidget);
    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyWidget);
    emptyLayout->setAlignment(Qt::AlignCenter);

    m_emptyLabel = new DLabel(emptyWidget);
    m_emptyLabel->setText("暂无待办事项\n输入内容后按回车添加");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    // 使用 DTK 主题色设置空态提示文字颜色
    DPalette pa = DPaletteHelper::instance()->palette(m_emptyLabel);
    pa.setColor(DPalette::WindowText, pa.color(DPalette::PlaceholderText));
    DPaletteHelper::instance()->setPalette(m_emptyLabel, pa);
    m_emptyLabel->setWordWrap(true);

    emptyLayout->addWidget(m_emptyLabel);

    m_stackWidget->addWidget(m_listContainer);
    m_stackWidget->addWidget(emptyWidget);

    mainLayout->addWidget(m_stackWidget, 1);

    // 底部统计
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    m_countLabel = new DLabel("共 0 项", this);
    bottomLayout->addWidget(m_countLabel);
    bottomLayout->addStretch();

    mainLayout->addLayout(bottomLayout);

    // 右键菜单
    m_contextMenu = new DMenu(this);
    QAction *toggleAction = m_contextMenu->addAction("切换完成状态");
    QAction *deleteAction = m_contextMenu->addAction("删除");

    // 连接信号
    connect(addBtn, &DPushButton::clicked, this, &TodoWidget::addTodo);
    connect(m_inputEdit, &DLineEdit::returnPressed, this, &TodoWidget::onReturnPressed);
    connect(m_listView, &DListView::doubleClicked, this, &TodoWidget::toggleCompleted);
    connect(m_listView, &DListView::customContextMenuRequested, this, &TodoWidget::showContextMenu);
    connect(toggleAction, &QAction::triggered, this, [this]() {
        if (m_contextIndex.isValid()) {
            toggleCompleted(m_contextIndex);
        }
    });
    connect(deleteAction, &QAction::triggered, this, &TodoWidget::removeTodo);

    updateCountLabel();
    updateEmptyState();
}

void TodoWidget::addTodo()
{
    QString text = m_inputEdit->text().trimmed();
    if (text.isEmpty()) {
        return;
    }

    TodoItem item(text);
    item.setId(m_nextId++);
    m_todos.append(item);

    QStandardItem *modelItem = new QStandardItem(text);
    modelItem->setCheckable(true);
    modelItem->setCheckState(Qt::Unchecked);
    m_model->appendRow(modelItem);

    m_inputEdit->clear();
    updateCountLabel();
    updateEmptyState();
    saveTodos();
}

void TodoWidget::removeTodo()
{
    if (!m_contextIndex.isValid()) return;

    int row = m_contextIndex.row();
    if (row >= 0 && row < m_todos.size()) {
        m_todos.removeAt(row);
    }
    m_model->removeRow(row);

    updateCountLabel();
    updateEmptyState();
    saveTodos();
}

void TodoWidget::toggleCompleted(const QModelIndex &index)
{
    if (!index.isValid()) return;

    int row = index.row();
    if (row >= 0 && row < m_todos.size()) {
        bool completed = !m_todos[row].completed();
        m_todos[row].setCompleted(completed);

        QStandardItem *item = m_model->item(row);
        if (item) {
            item->setCheckState(completed ? Qt::Checked : Qt::Unchecked);
            QFont font = item->font();
            font.setStrikeOut(completed);
            item->setFont(font);

            // 使用 DTK 主题色区分完成状态的前景色
            // 已完成项使用 PlaceholderText 颜色（次要文字色）
            DPalette pa = DPaletteHelper::instance()->palette(m_listView);
            if (completed) {
                item->setForeground(pa.color(DPalette::PlaceholderText));
            } else {
                item->setForeground(pa.color(DPalette::Text));
            }
        }
    }
    updateCountLabel();
    saveTodos();
}

void TodoWidget::onReturnPressed()
{
    addTodo();
}

void TodoWidget::showContextMenu(const QPoint &pos)
{
    QModelIndex index = m_listView->indexAt(pos);
    if (index.isValid()) {
        m_contextIndex = index;
        m_contextMenu->exec(m_listView->viewport()->mapToGlobal(pos));
    }
}

void TodoWidget::updateCountLabel()
{
    int total = m_todos.size();
    int completed = 0;
    for (const auto &item : m_todos) {
        if (item.completed()) completed++;
    }
    m_countLabel->setText(QString("共 %1 项 | 已完成 %2 项").arg(total).arg(completed));
}

void TodoWidget::updateEmptyState()
{
    if (m_model->rowCount() == 0) {
        m_stackWidget->setCurrentWidget(m_emptyLabel->parentWidget());
    } else {
        m_stackWidget->setCurrentWidget(m_listContainer);
    }
}

void TodoWidget::saveTodos()
{
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

            QStandardItem *modelItem = new QStandardItem(item.text());
            modelItem->setCheckable(true);
            modelItem->setCheckState(item.completed() ? Qt::Checked : Qt::Unchecked);
            if (item.completed()) {
                QFont font = modelItem->font();
                font.setStrikeOut(true);
                modelItem->setFont(font);
            }
            m_model->appendRow(modelItem);
        }
    }
    updateCountLabel();
    updateEmptyState();
}
