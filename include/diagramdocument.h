#pragma once

#include "diagramtypes.h"

#include <QStringList>

#include <map>

namespace flowchart {

class DiagramDocument {
public:
    void clear();

    bool contains(const QString& id) const;
    const DiagramItemModel* item(const QString& id) const;
    const NodeModel* node(const QString& id) const;
    const ConnectorModel* connector(const QString& id) const;

    void upsert(std::unique_ptr<DiagramItemModel> item);
    std::unique_ptr<DiagramItemModel> take(const QString& id);

    QStringList itemIds() const;
    QStringList connectorsForNode(const QString& nodeId) const;

    std::unique_ptr<DiagramItemModel> cloneItem(const QString& id) const;
    std::vector<std::unique_ptr<DiagramItemModel>> cloneItems(const QStringList& ids) const;
    std::vector<std::unique_ptr<DiagramItemModel>> cloneAll() const;

private:
    std::map<QString, std::unique_ptr<DiagramItemModel>> m_items;
};

}  // namespace flowchart
