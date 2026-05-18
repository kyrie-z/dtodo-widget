#include "todoitem.h"

TodoItem::TodoItem()
    : m_id(-1)
    , m_completed(false)
    , m_createdAt(QDateTime::currentDateTime())
{
}

TodoItem::TodoItem(const QString &text)
    : m_id(-1)
    , m_text(text)
    , m_completed(false)
    , m_createdAt(QDateTime::currentDateTime())
{
}

QString TodoItem::text() const { return m_text; }
void TodoItem::setText(const QString &text) { m_text = text; }

bool TodoItem::completed() const { return m_completed; }
void TodoItem::setCompleted(bool completed) { m_completed = completed; }

QDateTime TodoItem::createdAt() const { return m_createdAt; }
void TodoItem::setCreatedAt(const QDateTime &createdAt) { m_createdAt = createdAt; }

int TodoItem::id() const { return m_id; }
void TodoItem::setId(int id) { m_id = id; }
