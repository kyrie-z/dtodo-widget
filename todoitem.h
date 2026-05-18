#ifndef TODOITEM_H
#define TODOITEM_H

#include <QString>
#include <QDateTime>

class TodoItem
{
public:
    TodoItem();
    TodoItem(const QString &text);

    QString text() const;
    void setText(const QString &text);

    bool completed() const;
    void setCompleted(bool completed);

    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime &createdAt);

    int id() const;
    void setId(int id);

private:
    int m_id;
    QString m_text;
    bool m_completed;
    QDateTime m_createdAt;
};

#endif // TODOITEM_H
