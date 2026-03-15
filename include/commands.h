#pragma once

#include "diagramtypes.h"

#include <QUndoCommand>
#include <QStringList>

namespace flowchart {

class EditorSession;

class AddItemsCommand final : public QUndoCommand {
public:
    AddItemsCommand(
        EditorSession* session,
        std::vector<std::unique_ptr<DiagramItemModel>> items,
        const QString& text);

    void undo() override;
    void redo() override;

private:
    EditorSession* m_session;
    std::vector<std::unique_ptr<DiagramItemModel>> m_items;
    QStringList m_itemIds;
};

class RemoveItemsCommand final : public QUndoCommand {
public:
    RemoveItemsCommand(
        EditorSession* session,
        std::vector<std::unique_ptr<DiagramItemModel>> items,
        const QString& text);

    void undo() override;
    void redo() override;

private:
    EditorSession* m_session;
    std::vector<std::unique_ptr<DiagramItemModel>> m_items;
    QStringList m_itemIds;
};

class UpdateItemsCommand final : public QUndoCommand {
public:
    UpdateItemsCommand(
        EditorSession* session,
        std::vector<std::unique_ptr<DiagramItemModel>> before,
        std::vector<std::unique_ptr<DiagramItemModel>> after,
        const QString& text);

    void undo() override;
    void redo() override;

private:
    EditorSession* m_session;
    std::vector<std::unique_ptr<DiagramItemModel>> m_before;
    std::vector<std::unique_ptr<DiagramItemModel>> m_after;
};

}  // namespace flowchart
