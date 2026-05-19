#ifndef TODOWIDGET_H
#define TODOWIDGET_H

#include <DWidget>
#include <DLineEdit>
#include <DPushButton>
#include <DListView>
#include <DLabel>
#include <DBlurEffectWidget>
#include <DMenu>
#include <QStandardItemModel>
#include <QStackedWidget>

#include "todoitem.h"

DWIDGET_USE_NAMESPACE

class TodoWidget : public DBlurEffectWidget
{
    Q_OBJECT

public:
    explicit TodoWidget(QWidget *parent = nullptr);

private Q_SLOTS:
    void addTodo();
    void removeTodo();
    void toggleCompleted(const QModelIndex &index);
    void onReturnPressed();
    void showContextMenu(const QPoint &pos);

private:
    void setupUI();
    void updateCountLabel();
    void saveTodos();
    void loadTodos();
    void updateEmptyState();

    DLineEdit *m_inputEdit;
    DListView *m_listView;
    QStandardItemModel *m_model;
    DLabel *m_countLabel;
    DLabel *m_emptyLabel;
    QStackedWidget *m_stackWidget;
    QWidget *m_listContainer;
    DMenu *m_contextMenu;
    QModelIndex m_contextIndex;
    QList<TodoItem> m_todos;
    int m_nextId;
};

#endif // TODOWIDGET_H
