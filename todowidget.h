#ifndef TODOWIDGET_H
#define TODOWIDGET_H

#include <DWidget>
#include <DLineEdit>
#include <DPushButton>
#include <DListView>
#include <DLabel>
#include <DBlurEffectWidget>
#include <QStandardItemModel>

#include "todoitem.h"

DWIDGET_USE_NAMESPACE

class TodoWidget : public DBlurEffectWidget
{
    Q_OBJECT

public:
    explicit TodoWidget(QWidget *parent = nullptr);

private Q_SLOTS:
    void addTodo();
    void removeTodo(const QModelIndex &index);
    void toggleCompleted(const QModelIndex &index);
    void onReturnPressed();

private:
    void setupUI();
    void updateCountLabel();
    void saveTodos();
    void loadTodos();

    DLineEdit *m_inputEdit;
    DListView *m_listView;
    QStandardItemModel *m_model;
    DLabel *m_countLabel;
    QList<TodoItem> m_todos;
    int m_nextId;
};

#endif // TODOWIDGET_H
