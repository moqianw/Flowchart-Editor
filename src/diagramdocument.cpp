#include "diagramdocument.h"

#include <algorithm>

namespace flowchart {

void DiagramDocument::clear() {
    m_items.clear();
}

bool DiagramDocument::contains(const QString& id) const {
    return m_items.find(id) != m_items.end();
}

const DiagramItemModel* DiagramDocument::item(const QString& id) const {
    const auto it = m_items.find(id);
    return it == m_items.end() ? nullptr : it->second.get();
}

const NodeModel* DiagramDocument::node(const QString& id) const {
    const auto* found = item(id);
    if (!found || found->kind() != ItemKind::Node) {
        return nullptr;
    }
    return static_cast<const NodeModel*>(found);
}

const ConnectorModel* DiagramDocument::connector(const QString& id) const {
    const auto* found = item(id);
    if (!found || found->kind() != ItemKind::Connector) {
        return nullptr;
    }
    return static_cast<const ConnectorModel*>(found);
}

void DiagramDocument::upsert(std::unique_ptr<DiagramItemModel> item) {
    m_items[item->id] = std::move(item);
}

std::unique_ptr<DiagramItemModel> DiagramDocument::take(const QString& id) {
    const auto it = m_items.find(id);
    if (it == m_items.end()) {
        return nullptr;
    }

    auto item = std::move(it->second);
    m_items.erase(it);
    return item;
}

QStringList DiagramDocument::itemIds() const {
    QStringList ids;
    ids.reserve(static_cast<int>(m_items.size()));
    for (const auto& [id, item] : m_items) {
        Q_UNUSED(item);
        ids.push_back(id);
    }
    return ids;
}

QStringList DiagramDocument::connectorsForNode(const QString& nodeId) const {
    QStringList ids;
    for (const auto& [id, item] : m_items) {
        if (item->kind() != ItemKind::Connector) {
            continue;
        }

        const auto* connector = static_cast<const ConnectorModel*>(item.get());
        if (connector->start.itemId == nodeId || connector->end.itemId == nodeId) {
            ids.push_back(id);
        }
    }
    return ids;
}

std::unique_ptr<DiagramItemModel> DiagramDocument::cloneItem(const QString& id) const {
    const auto* found = item(id);
    return found ? found->clone() : nullptr;
}

std::vector<std::unique_ptr<DiagramItemModel>> DiagramDocument::cloneItems(
    const QStringList& ids) const {
    std::vector<std::unique_ptr<DiagramItemModel>> result;
    result.reserve(ids.size());
    for (const QString& id : ids) {
        if (const auto* found = item(id)) {
            result.push_back(found->clone());
        }
    }
    return result;
}

std::vector<std::unique_ptr<DiagramItemModel>> DiagramDocument::cloneAll() const {
    std::vector<const DiagramItemModel*> sorted;
    sorted.reserve(m_items.size());
    for (const auto& [id, item] : m_items) {
        Q_UNUSED(id);
        sorted.push_back(item.get());
    }

    std::sort(sorted.begin(), sorted.end(), [](const DiagramItemModel* left, const DiagramItemModel* right) {
        if (left->kind() != right->kind()) {
            return left->kind() == ItemKind::Node;
        }
        if (!qFuzzyCompare(left->zValue + 1.0, right->zValue + 1.0)) {
            return left->zValue < right->zValue;
        }
        return left->id < right->id;
    });

    std::vector<std::unique_ptr<DiagramItemModel>> result;
    result.reserve(sorted.size());
    for (const DiagramItemModel* item : sorted) {
        result.push_back(item->clone());
    }
    return result;
}

}  // namespace flowchart
