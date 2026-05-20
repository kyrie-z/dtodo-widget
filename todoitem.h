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

    QString color() const;
    void setColor(const QString &color);
    bool hasColor() const;

private:
    int m_id;
    QString m_text;
    bool m_completed;
    QDateTime m_createdAt;
    QString m_color;  ///< 颜色标记，空字符串表示无颜色，否则为十六进制如 "#e53935"
};

#endif // TODOITEM_H
