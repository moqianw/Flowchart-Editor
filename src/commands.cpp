#include "commands.h"

#include "editorsession.h"

namespace flowchart {

AddItemsCommand::AddItemsCommand(
    EditorSession* session,
    std::vector<std::unique_ptr<DiagramItemModel>> items,
    const QString& text)
    : QUndoCommand(text)
    , m_session(session)
    , m_items(std::move(items)) {
    for (const auto& item : m_items) {
        m_itemIds.push_back(item->id);
    }
}

void AddItemsCommand::undo() {
    m_session->removeItemsInternal(m_itemIds);
}

void AddItemsCommand::redo() {
    m_session->insertItemsInternal(m_items);
}

RemoveItemsCommand::RemoveItemsCommand(
    EditorSession* session,
    std::vector<std::unique_ptr<DiagramItemModel>> items,
    const QString& text)
    : QUndoCommand(text)
    , m_session(session)
    , m_items(std::move(items)) {
    for (const auto& item : m_items) {
        m_itemIds.push_back(item->id);
    }
}

void RemoveItemsCommand::undo() {
    m_session->insertItemsInternal(m_items);
}

void RemoveItemsCommand::redo() {
    m_session->removeItemsInternal(m_itemIds);
}

UpdateItemsCommand::UpdateItemsCommand(
    EditorSession* session,
    std::vector<std::unique_ptr<DiagramItemModel>> before,
    std::vector<std::unique_ptr<DiagramItemModel>> after,
    const QString& text)
    : QUndoCommand(text)
    , m_session(session)
    , m_before(std::move(before))
    , m_after(std::move(after)) {}

void UpdateItemsCommand::undo() {
    m_session->replaceItemsInternal(m_before);
}

void UpdateItemsCommand::redo() {
    m_session->replaceItemsInternal(m_after);
}

}  // namespace flowchart
