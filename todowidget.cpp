#include "todowidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>

DWIDGET_USE_NAMESPACE

TodoWidget::TodoWidget(QWidget *parent)
    : DBlurEffectWidget(parent)
    , m_nextId(1)
{
    // 设置模糊特效
    setMode(DBlurEffectWidget::GaussianBlur);
    setBlendMode(DBlurEffectWidget::InWidgetBlend);
    setMaskAlpha(0);
    setRadius(20);
    setBlurRectXRadius(12);
    setBlurRectYRadius(12);
    
    setupUI();
    loadTodos();
}

void TodoWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    // 输入区域
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(8);

    m_inputEdit = new DLineEdit(this);
    m_inputEdit->setPlaceholderText("添加新的待办事项...");
    m_inputEdit->setClearButtonEnabled(true);

    DPushButton *addBtn = new DPushButton("添加", this);
    addBtn->setFixedSize(60, 36);

    inputLayout->addWidget(m_inputEdit);
    inputLayout->addWidget(addBtn);

    mainLayout->addLayout(inputLayout);

    // 列表区域
    m_listView = new DListView(this);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setBackgroundType(DStyledItemDelegate::RoundedBackground);
    m_listView->setItemSpacing(5);

    m_model = new QStandardItemModel(this);
    m_listView->setModel(m_model);

    mainLayout->addWidget(m_listView, 1);

    // 底部统计
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    m_countLabel = new DLabel("共 0 项", this);
    bottomLayout->addWidget(m_countLabel);
    bottomLayout->addStretch();

    mainLayout->addLayout(bottomLayout);

    // 连接信号
    connect(addBtn, &DPushButton::clicked, this, &TodoWidget::addTodo);
    connect(m_inputEdit, &DLineEdit::returnPressed, this, &TodoWidget::onReturnPressed);
    connect(m_listView, &DListView::doubleClicked, this, &TodoWidget::toggleCompleted);

    updateCountLabel();
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
    saveTodos();
}

void TodoWidget::removeTodo(const QModelIndex &index)
{
    if (!index.isValid()) return;

    int row = index.row();
    if (row >= 0 && row < m_todos.size()) {
        m_todos.removeAt(row);
    }
    m_model->removeRow(row);

    updateCountLabel();
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
        }
    }
    saveTodos();
}

void TodoWidget::onReturnPressed()
{
    addTodo();
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
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
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
        file.close();
    }
    updateCountLabel();
}
