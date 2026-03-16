#include "editorsession.h"

#include "canvasitems.h"
#include "commands.h"
#include "documentserializer.h"

#include <QGraphicsItem>
#include <QMap>
#include <QSet>

#include <algorithm>
#include <cmath>
#include <utility>

namespace flowchart {

namespace {

bool vectorChanged(
    const std::vector<std::unique_ptr<DiagramItemModel>>& before,
    const std::vector<std::unique_ptr<DiagramItemModel>>& after) {
    if (before.size() != after.size()) {
        return true;
    }
    for (std::size_t index = 0; index < before.size(); ++index) {
        if (!modelsEqual(*before[index], *after[index])) {
            return true;
        }
    }
    return false;
}

}  // namespace

bool PortHit::isValid() const {
    return !itemId.isEmpty() && portIndex >= 0;
}

EditorSession::EditorSession(QObject* parent)
    : QObject(parent)
    , m_scene(new QGraphicsScene(this))
    , m_undoStack(new QUndoStack(this)) {
    m_scene->setSceneRect(0.0, 0.0, 2400.0, 1600.0);

    connect(m_scene, &QGraphicsScene::selectionChanged, this, &EditorSession::selectionChanged);
    connect(m_undoStack, &QUndoStack::cleanChanged, this, [this](bool) {
        emitDirtyStateIfChanged();
    });
}

QGraphicsScene* EditorSession::scene() const {
    return m_scene;
}

QUndoStack* EditorSession::undoStack() const {
    return m_undoStack;
}

const ShapeRegistry& EditorSession::registry() const {
    return m_registry;
}

const DiagramDocument& EditorSession::document() const {
    return m_document;
}

std::vector<std::unique_ptr<DiagramItemModel>> EditorSession::exportDocumentSnapshot() const {
    return cloneAllWithResolvedEndpoints();
}

QStringList EditorSession::selectedItemIds() const {
    QStringList ids;
    for (QGraphicsItem* item : m_scene->selectedItems()) {
        auto* diagramItem = dynamic_cast<CanvasDiagramItem*>(item);
        if (!diagramItem) {
            continue;
        }
        ids.push_back(diagramItem->itemId());
    }
    ids.removeDuplicates();
    return ids;
}

void EditorSession::selectAll() {
    for (QGraphicsItem* item : std::as_const(m_viewItems)) {
        if (item) {
            item->setSelected(true);
        }
    }
}

bool EditorSession::hasSingleSelection() const {
    return selectedItemIds().size() == 1;
}

QString EditorSession::primarySelectedItemId() const {
    const QStringList ids = selectedItemIds();
    return ids.isEmpty() ? QString() : ids.front();
}

bool EditorSession::isDirty() const {
    return m_forceDirty || !m_undoStack->isClean();
}

void EditorSession::markClean() {
    m_forceDirty = false;
    m_undoStack->setClean();
    emitDirtyStateIfChanged();
}

void EditorSession::markDirty() {
    m_forceDirty = true;
    emitDirtyStateIfChanged();
}

void EditorSession::resetDocument() {
    resetDocumentInternal({});
}

void EditorSession::restoreRecoveredDocument(std::vector<std::unique_ptr<DiagramItemModel>> items) {
    resetDocumentInternal(std::move(items));
    markDirty();
}

bool EditorSession::loadFromFile(const QString& fileName, QString* errorMessage) {
    QString localError;
    auto items = DocumentSerializer::load(fileName, &localError);
    if (!localError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = localError;
        }
        return false;
    }

    resetDocumentInternal(std::move(items));
    return true;
}

bool EditorSession::saveToFile(const QString& fileName, QString* errorMessage) const {
    return DocumentSerializer::save(cloneAllWithResolvedEndpoints(), fileName, errorMessage);
}

bool EditorSession::reloadShapeRegistry(QStringList* warnings) {
    if (!m_registry.reload(warnings)) {
        // Continue even with warnings so valid components still load.
    }

    const QStringList itemIds = m_document.itemIds();
    const QStringList selectedIds = selectedItemIds();
    for (const QString& itemId : itemIds) {
        removeViewForItem(itemId);
    }
    for (const QString& itemId : itemIds) {
        createViewForItem(itemId);
        syncViewForItem(itemId);
    }
    refreshAllConnectors();

    m_scene->clearSelection();
    for (const QString& itemId : selectedIds) {
        if (QGraphicsItem* item = m_viewItems.value(itemId, nullptr)) {
            item->setSelected(true);
        }
    }

    emit documentChanged();
    return warnings == nullptr || warnings->isEmpty();
}

void EditorSession::createItem(const QString& typeId, const QPointF& scenePos) {
    const ShapeDefinition* definition = m_registry.definition(typeId);
    if (!definition) {
        return;
    }

    std::vector<std::unique_ptr<DiagramItemModel>> items;
    items.reserve(1);

    auto node = std::make_unique<NodeModel>();
    node->id = createItemId();
    node->typeId = definition->typeId;
    node->rect = snapRect(QRectF(scenePos, definition->defaultSize));
    node->props = definition->defaultProps;
    node->style = ItemStyle{};
    if (definition->initializeStyle) {
        definition->initializeStyle(node->style);
    }
    items.push_back(std::move(node));

    m_undoStack->push(new AddItemsCommand(this, std::move(items), QStringLiteral("Create item")));
}

void EditorSession::createConnector(const QPointF& scenePos) {
    std::vector<std::unique_ptr<DiagramItemModel>> items;
    items.reserve(1);

    auto connector = std::make_unique<ConnectorModel>();
    connector->id = createItemId();
    connector->typeId = QStringLiteral("ArrowConnector");
    connector->style = ItemStyle{};
    connector->style.fillColor = connector->style.strokeColor;
    connector->start.position = snapPoint(scenePos);
    connector->end.position = snapPoint(scenePos + QPointF(160.0, 0.0));
    connector->props.insert(QStringLiteral("headStyle"), QStringLiteral("stealth"));
    connector->props.insert(QStringLiteral("lineStyle"), QStringLiteral("solid"));
    items.push_back(std::move(connector));

    m_undoStack->push(new AddItemsCommand(this, std::move(items), QStringLiteral("Create connector")));
}

void EditorSession::resetSelectedConnectorBends() {
    applyToItems(selectedItemIds(), QStringLiteral("Auto route connectors"), [](DiagramItemModel& item) {
        if (item.kind() != ItemKind::Connector) {
            return;
        }
        auto& connector = static_cast<ConnectorModel&>(item);
        connector.props.remove(QStringLiteral("bendX"));
        connector.props.remove(QStringLiteral("bendY"));
    });
}

void EditorSession::routeSelectedConnectorsVertical() {
    applyToItems(selectedItemIds(), QStringLiteral("Route connectors vertically"), [this](DiagramItemModel& item) {
        if (item.kind() != ItemKind::Connector) {
            return;
        }
        auto& connector = static_cast<ConnectorModel&>(item);
        const QPointF start = resolveEndpoint(connector.start);
        const QPointF end = resolveEndpoint(connector.end);
        connector.props.remove(QStringLiteral("bendY"));
        connector.props.insert(QStringLiteral("bendX"), snapCoordinate((start.x() + end.x()) / 2.0));
    });
}

void EditorSession::routeSelectedConnectorsHorizontal() {
    applyToItems(selectedItemIds(), QStringLiteral("Route connectors horizontally"), [this](DiagramItemModel& item) {
        if (item.kind() != ItemKind::Connector) {
            return;
        }
        auto& connector = static_cast<ConnectorModel&>(item);
        const QPointF start = resolveEndpoint(connector.start);
        const QPointF end = resolveEndpoint(connector.end);
        connector.props.remove(QStringLiteral("bendX"));
        connector.props.insert(QStringLiteral("bendY"), snapCoordinate((start.y() + end.y()) / 2.0));
    });
}

void EditorSession::setSelectedConnectorHeadStyle(const QString& headStyle) {
    applyToItems(selectedItemIds(), QStringLiteral("Change connector head style"), [headStyle](DiagramItemModel& item) {
        if (item.kind() != ItemKind::Connector) {
            return;
        }
        auto& connector = static_cast<ConnectorModel&>(item);
        connector.props.insert(QStringLiteral("headStyle"), headStyle);
    });
}

void EditorSession::setSelectedConnectorLineStyle(const QString& lineStyle) {
    applyToItems(selectedItemIds(), QStringLiteral("Change connector line style"), [lineStyle](DiagramItemModel& item) {
        if (item.kind() != ItemKind::Connector) {
            return;
        }
        auto& connector = static_cast<ConnectorModel&>(item);
        connector.props.insert(QStringLiteral("lineStyle"), lineStyle);
    });
}

void EditorSession::deleteSelected() {
    const QStringList ids = expandSelectionForDeletion(selectedItemIds());
    if (ids.isEmpty()) {
        return;
    }

    m_undoStack->push(new RemoveItemsCommand(
        this,
        cloneItemsWithResolvedEndpoints(ids),
        QStringLiteral("Delete items")));
}

void EditorSession::copySelected() {
    m_clipboard = cloneItemsWithResolvedEndpoints(selectedItemIds());
}

void EditorSession::cutSelected() {
    copySelected();
    deleteSelected();
}

void EditorSession::paste(const QPointF& scenePos) {
    if (m_clipboard.empty()) {
        return;
    }

    auto items = cloneModels(m_clipboard);
    QHash<QString, QString> idMap;
    for (auto& item : items) {
        const QString oldId = item->id;
        item->id = createItemId();
        idMap.insert(oldId, item->id);
    }

    const QRectF sourceRect = itemsBoundingRect(items);
    const QPointF offset = sourceRect.isEmpty() ? QPointF(20.0, 20.0) : (scenePos - sourceRect.topLeft());

    for (auto& item : items) {
        if (item->kind() == ItemKind::Node) {
            auto* node = static_cast<NodeModel*>(item.get());
            node->rect.translate(offset);
            continue;
        }

        auto* connector = static_cast<ConnectorModel*>(item.get());
        connector->start.position += offset;
        connector->end.position += offset;

        if (idMap.contains(connector->start.itemId)) {
            connector->start.itemId = idMap.value(connector->start.itemId);
        } else {
            connector->start.itemId.clear();
            connector->start.portIndex = -1;
        }

        if (idMap.contains(connector->end.itemId)) {
            connector->end.itemId = idMap.value(connector->end.itemId);
        } else {
            connector->end.itemId.clear();
            connector->end.portIndex = -1;
        }
    }

    m_undoStack->push(new AddItemsCommand(this, std::move(items), QStringLiteral("Paste items")));
}

void EditorSession::applyStrokeColor(const QColor& color) {
    applyToItems(selectedItemIds(), QStringLiteral("Change stroke color"), [color](DiagramItemModel& item) {
        item.style.strokeColor = color;
    });
}

void EditorSession::applyFillColor(const QColor& color) {
    applyToItems(selectedItemIds(), QStringLiteral("Change fill color"), [color](DiagramItemModel& item) {
        item.style.fillColor = color;
    });
}

void EditorSession::applyFillAlpha(int alpha) {
    applyToItems(selectedItemIds(), QStringLiteral("Change fill opacity"), [alpha](DiagramItemModel& item) {
        QColor updated = item.style.fillColor;
        updated.setAlpha(alpha);
        item.style.fillColor = updated;
    });
}

void EditorSession::applyStrokeWidth(int width) {
    applyToItems(selectedItemIds(), QStringLiteral("Change stroke width"), [width](DiagramItemModel& item) {
        item.style.strokeWidth = width;
    });
}

void EditorSession::applyFontFamily(const QFont& font) {
    applyToItems(selectedItemIds(), QStringLiteral("Change font family"), [font](DiagramItemModel& item) {
        QFont updated = item.style.font;
        updated.setFamily(font.family());
        item.style.font = updated;
    });
}

void EditorSession::applyFontSize(int pointSize) {
    applyToItems(selectedItemIds(), QStringLiteral("Change font size"), [pointSize](DiagramItemModel& item) {
        item.style.font.setPointSize(pointSize);
    });
}

void EditorSession::toggleBold() {
    applyToItems(selectedItemIds(), QStringLiteral("Toggle bold"), [](DiagramItemModel& item) {
        item.style.bold = !item.style.bold;
    });
}

void EditorSession::toggleItalic() {
    applyToItems(selectedItemIds(), QStringLiteral("Toggle italic"), [](DiagramItemModel& item) {
        item.style.italic = !item.style.italic;
    });
}

void EditorSession::toggleUnderline() {
    applyToItems(selectedItemIds(), QStringLiteral("Toggle underline"), [](DiagramItemModel& item) {
        item.style.underline = !item.style.underline;
    });
}

void EditorSession::rotateSelected(qreal deltaDegrees) {
    applyToItems(selectedItemIds(), QStringLiteral("Rotate items"), [deltaDegrees](DiagramItemModel& item) {
        if (item.kind() == ItemKind::Node) {
            item.style.rotation += deltaDegrees;
        }
    });
}

void EditorSession::scaleSelected(qreal factor) {
    applyToItems(selectedItemIds(), QStringLiteral("Scale items"), [factor](DiagramItemModel& item) {
        if (item.kind() == ItemKind::Node) {
            item.style.scale = std::clamp(item.style.scale * factor, 0.2, 8.0);
        }
    });
}

void EditorSession::bringForward() {
    applyToItems(selectedItemIds(), QStringLiteral("Bring forward"), [](DiagramItemModel& item) {
        item.zValue += 1.0;
    });
}

void EditorSession::sendBackward() {
    applyToItems(selectedItemIds(), QStringLiteral("Send backward"), [](DiagramItemModel& item) {
        item.zValue -= 1.0;
    });
}

void EditorSession::setText(const QStringList& itemIds, const QString& text) {
    applyToItems(itemIds, QStringLiteral("Edit text"), [text](DiagramItemModel& item) {
        item.text = text;
    });
}

void EditorSession::updateNodeGeometry(
    const QString& itemId,
    const QRectF& rect,
    qreal rotation,
    qreal scale) {
    const auto* current = m_document.node(itemId);
    if (!current) {
        return;
    }

    auto before = cloneItemsWithResolvedEndpoints(QStringList{itemId});
    auto after = cloneModels(before);
    auto* node = static_cast<NodeModel*>(after.front().get());
    node->rect = snapRect(rect.normalized());
    node->style.rotation = rotation;
    node->style.scale = scale;

    if (!vectorChanged(before, after)) {
        return;
    }

    m_undoStack->push(new UpdateItemsCommand(
        this,
        std::move(before),
        std::move(after),
        QStringLiteral("Update node properties")));
}

void EditorSession::autoLayoutSelection() {
    QStringList nodeIds = selectedItemIds();
    if (nodeIds.isEmpty()) {
        nodeIds = m_document.itemIds();
    }

    nodeIds.erase(
        std::remove_if(nodeIds.begin(), nodeIds.end(), [this](const QString& id) {
            return m_document.node(id) == nullptr;
        }),
        nodeIds.end());
    nodeIds.removeDuplicates();

    if (nodeIds.size() < 2) {
        return;
    }

    const QSet<QString> nodeSet(nodeIds.begin(), nodeIds.end());
    QHash<QString, QVector<QString>> outgoing;
    QHash<QString, int> indegree;
    for (const QString& nodeId : nodeIds) {
        outgoing.insert(nodeId, {});
        indegree.insert(nodeId, 0);
    }

    for (const QString& itemId : m_document.itemIds()) {
        const auto* connector = m_document.connector(itemId);
        if (!connector || !connector->start.isAttached() || !connector->end.isAttached()) {
            continue;
        }
        if (!nodeSet.contains(connector->start.itemId) || !nodeSet.contains(connector->end.itemId)) {
            continue;
        }
        if (connector->start.itemId == connector->end.itemId) {
            continue;
        }
        outgoing[connector->start.itemId].push_back(connector->end.itemId);
        indegree[connector->end.itemId] += 1;
    }

    auto nodePositionLess = [this](const QString& leftId, const QString& rightId) {
        const auto* left = m_document.node(leftId);
        const auto* right = m_document.node(rightId);
        if (!left || !right) {
            return leftId < rightId;
        }
        if (!qFuzzyCompare(left->rect.top() + 1.0, right->rect.top() + 1.0)) {
            return left->rect.top() < right->rect.top();
        }
        if (!qFuzzyCompare(left->rect.left() + 1.0, right->rect.left() + 1.0)) {
            return left->rect.left() < right->rect.left();
        }
        return leftId < rightId;
    };

    QStringList queue;
    for (const QString& nodeId : nodeIds) {
        if (indegree.value(nodeId) == 0) {
            queue.push_back(nodeId);
        }
    }
    std::sort(queue.begin(), queue.end(), nodePositionLess);

    QHash<QString, int> layerByNode;
    QStringList ordered;
    while (!queue.isEmpty()) {
        const QString current = queue.takeFirst();
        ordered.push_back(current);
        for (const QString& next : outgoing.value(current)) {
            layerByNode[next] = std::max(layerByNode.value(next, 0), layerByNode.value(current, 0) + 1);
            indegree[next] -= 1;
            if (indegree[next] == 0) {
                queue.push_back(next);
            }
        }
        std::sort(queue.begin(), queue.end(), nodePositionLess);
    }

    QStringList remaining;
    for (const QString& nodeId : nodeIds) {
        if (!ordered.contains(nodeId)) {
            remaining.push_back(nodeId);
        }
    }
    std::sort(remaining.begin(), remaining.end(), nodePositionLess);

    int fallbackLayer = 0;
    for (const QString& nodeId : ordered) {
        fallbackLayer = std::max(fallbackLayer, layerByNode.value(nodeId, 0));
    }
    for (const QString& nodeId : remaining) {
        layerByNode[nodeId] = ++fallbackLayer;
        ordered.push_back(nodeId);
    }

    QMap<int, QStringList> layers;
    for (const QString& nodeId : ordered) {
        layers[layerByNode.value(nodeId, 0)].push_back(nodeId);
    }
    for (auto it = layers.begin(); it != layers.end(); ++it) {
        std::sort(it.value().begin(), it.value().end(), nodePositionLess);
    }

    auto before = cloneItemsWithResolvedEndpoints(nodeIds);
    auto after = cloneModels(before);
    QHash<QString, NodeModel*> byId;
    for (auto& item : after) {
        byId.insert(item->id, static_cast<NodeModel*>(item.get()));
    }

    constexpr qreal kStartX = 80.0;
    constexpr qreal kStartY = 80.0;
    constexpr qreal kColumnGap = 120.0;
    constexpr qreal kRowGap = 80.0;

    qreal currentX = kStartX;
    for (auto it = layers.begin(); it != layers.end(); ++it) {
        qreal currentY = kStartY;
        qreal layerWidth = 0.0;
        for (const QString& nodeId : it.value()) {
            NodeModel* node = byId.value(nodeId, nullptr);
            if (!node) {
                continue;
            }
            QRectF rect = node->rect;
            rect.moveTopLeft(snapPoint(QPointF(currentX, currentY)));
            node->rect = rect;
            currentY += rect.height() + kRowGap;
            layerWidth = std::max(layerWidth, rect.width());
        }
        currentX += layerWidth + kColumnGap;
    }

    if (!vectorChanged(before, after)) {
        return;
    }

    m_undoStack->push(new UpdateItemsCommand(
        this,
        std::move(before),
        std::move(after),
        QStringLiteral("Auto layout")));
}

QStringList EditorSession::validateDocument() const {
    QStringList issues;
    QStringList nodeIds;
    QHash<QString, int> indegree;
    QHash<QString, int> outdegree;
    int terminatorCount = 0;

    for (const QString& itemId : m_document.itemIds()) {
        const auto* node = m_document.node(itemId);
        if (!node) {
            continue;
        }
        if (node->typeId == QStringLiteral("Textpointer")) {
            continue;
        }
        nodeIds.push_back(itemId);
        indegree.insert(itemId, 0);
        outdegree.insert(itemId, 0);
        if (node->typeId == QStringLiteral("Start_or_Terminator")) {
            ++terminatorCount;
        }
    }

    if (nodeIds.isEmpty()) {
        issues.push_back(QStringLiteral("画布中没有节点。"));
        return issues;
    }

    if (terminatorCount == 0) {
        issues.push_back(QStringLiteral("未检测到开始/结束节点。"));
    }

    QSet<QString> connectedNodes;
    int freeConnectorCount = 0;
    int selfLoopCount = 0;

    for (const QString& itemId : m_document.itemIds()) {
        const auto* connector = m_document.connector(itemId);
        if (!connector) {
            continue;
        }

        if (!connector->start.isAttached() || !connector->end.isAttached()) {
            ++freeConnectorCount;
            continue;
        }

        connectedNodes.insert(connector->start.itemId);
        connectedNodes.insert(connector->end.itemId);

        if (connector->start.itemId == connector->end.itemId) {
            ++selfLoopCount;
            continue;
        }

        outdegree[connector->start.itemId] += 1;
        indegree[connector->end.itemId] += 1;
    }

    if (freeConnectorCount > 0) {
        issues.push_back(QStringLiteral("存在 %1 条未完整连接的连线。").arg(freeConnectorCount));
    }
    if (selfLoopCount > 0) {
        issues.push_back(QStringLiteral("存在 %1 条自环连线。").arg(selfLoopCount));
    }

    int isolatedCount = 0;
    for (const QString& nodeId : nodeIds) {
        const auto* node = m_document.node(nodeId);
        if (!node || node->typeId == QStringLiteral("Textpointer")) {
            continue;
        }
        if (!connectedNodes.contains(nodeId)) {
            ++isolatedCount;
        }
    }
    if (isolatedCount > 0) {
        issues.push_back(QStringLiteral("存在 %1 个未连接的节点。").arg(isolatedCount));
    }

    int entryCount = 0;
    int terminalNodes = 0;
    for (const QString& nodeId : nodeIds) {
        if (indegree.value(nodeId) == 0) {
            ++entryCount;
        }
        if (outdegree.value(nodeId) == 0) {
            ++terminalNodes;
        }
    }

    if (entryCount == 0) {
        issues.push_back(QStringLiteral("未检测到入口节点，图中可能存在环路。"));
    } else if (entryCount > 1) {
        issues.push_back(QStringLiteral("检测到 %1 个入口节点。").arg(entryCount));
    }

    if (terminalNodes == 0) {
        issues.push_back(QStringLiteral("未检测到出口节点。"));
    } else if (terminalNodes > 1) {
        issues.push_back(QStringLiteral("检测到 %1 个出口节点。").arg(terminalNodes));
    }

    QHash<QString, int> indegreeCopy = indegree;
    QStringList queue;
    for (const QString& nodeId : nodeIds) {
        if (indegreeCopy.value(nodeId) == 0) {
            queue.push_back(nodeId);
        }
    }

    int visitedCount = 0;
    while (!queue.isEmpty()) {
        const QString current = queue.takeFirst();
        ++visitedCount;
        for (const QString& itemId : m_document.itemIds()) {
            const auto* connector = m_document.connector(itemId);
            if (!connector
                || !connector->start.isAttached()
                || !connector->end.isAttached()
                || connector->start.itemId != current
                || connector->start.itemId == connector->end.itemId) {
                continue;
            }
            indegreeCopy[connector->end.itemId] -= 1;
            if (indegreeCopy[connector->end.itemId] == 0) {
                queue.push_back(connector->end.itemId);
            }
        }
    }

    if (visitedCount < nodeIds.size()) {
        issues.push_back(QStringLiteral("检测到环路，建议检查分支回流。"));
    }

    return issues;
}

bool EditorSession::snapToGridEnabled() const {
    return m_snapToGridEnabled;
}

void EditorSession::setSnapToGridEnabled(bool enabled) {
    if (m_snapToGridEnabled == enabled) {
        return;
    }
    m_snapToGridEnabled = enabled;
    emit documentChanged();
}

int EditorSession::gridSize() const {
    return m_gridSize;
}

QPointF EditorSession::snapPoint(const QPointF& point) const {
    if (!m_snapToGridEnabled) {
        return point;
    }
    return QPointF(snapCoordinate(point.x()), snapCoordinate(point.y()));
}

QRectF EditorSession::snapRect(const QRectF& rect) const {
    if (!m_snapToGridEnabled) {
        return rect;
    }

    const QPointF topLeft = snapPoint(rect.topLeft());
    const QPointF bottomRight = snapPoint(rect.bottomRight());
    QRectF snapped(topLeft, bottomRight);

    if (snapped.width() < 1.0) {
        snapped.setWidth(std::max<qreal>(rect.width(), 1.0));
    }
    if (snapped.height() < 1.0) {
        snapped.setHeight(std::max<qreal>(rect.height(), 1.0));
    }

    return snapped.normalized();
}

void EditorSession::beginInteractiveChange(const QStringList& itemIds, const QString& description) {
    if (m_replayingCommand || itemIds.isEmpty() || m_pendingInteraction.has_value()) {
        return;
    }

    PendingInteraction interaction;
    interaction.description = description;
    interaction.itemIds = itemIds;
    interaction.before = cloneItemsWithResolvedEndpoints(itemIds);
    m_pendingInteraction = std::move(interaction);
}

bool EditorSession::isInteractiveChangeActiveFor(const QString& itemId) const {
    return m_pendingInteraction.has_value() && m_pendingInteraction->itemIds.contains(itemId);
}

void EditorSession::commitInteractiveChange() {
    if (!m_pendingInteraction.has_value()) {
        return;
    }

    auto interaction = std::move(*m_pendingInteraction);
    m_pendingInteraction.reset();

    const auto after = cloneItemsWithResolvedEndpoints(interaction.itemIds);
    if (!vectorChanged(interaction.before, after)) {
        return;
    }

    m_undoStack->push(new UpdateItemsCommand(
        this,
        std::move(interaction.before),
        cloneModels(after),
        interaction.description));
}

void EditorSession::applyNodeGeometryInteractive(
    const QString& itemId,
    const QRectF& rect,
    qreal rotation,
    qreal scale) {
    const auto* current = m_document.node(itemId);
    if (!current) {
        return;
    }

    auto updated = std::make_unique<NodeModel>(*current);
    updated->rect = snapRect(rect.normalized());
    updated->style.rotation = rotation;
    updated->style.scale = scale;
    m_document.upsert(std::move(updated));
    syncViewForItem(itemId);
    refreshConnectorsForNode(itemId);
    emit documentChanged();
}

void EditorSession::applyConnectorInteractive(
    const QString& itemId,
    const EndpointRef& start,
    const EndpointRef& end) {
    const auto* current = m_document.connector(itemId);
    if (!current) {
        return;
    }

    auto updated = std::make_unique<ConnectorModel>(*current);
    updated->start = start;
    updated->end = end;
    if (!updated->start.isAttached()) {
        updated->start.position = snapPoint(updated->start.position);
    }
    if (!updated->end.isAttached()) {
        updated->end.position = snapPoint(updated->end.position);
    }
    m_document.upsert(std::move(updated));
    syncViewForItem(itemId);
    emit documentChanged();
}

void EditorSession::applyConnectorBendInteractive(const QString& itemId, const QJsonObject& props) {
    const auto* current = m_document.connector(itemId);
    if (!current) {
        return;
    }

    auto updated = std::make_unique<ConnectorModel>(*current);
    updated->props = props;
    if (updated->props.contains(QStringLiteral("bendX"))) {
        updated->props.insert(
            QStringLiteral("bendX"),
            snapCoordinate(updated->props.value(QStringLiteral("bendX")).toDouble()));
    }
    if (updated->props.contains(QStringLiteral("bendY"))) {
        updated->props.insert(
            QStringLiteral("bendY"),
            snapCoordinate(updated->props.value(QStringLiteral("bendY")).toDouble()));
    }
    m_document.upsert(std::move(updated));
    syncViewForItem(itemId);
    emit documentChanged();
}

PortHit EditorSession::hitTestPort(const QPointF& scenePos, qreal threshold) const {
    PortHit bestHit;
    qreal bestDistance = threshold;

    for (QGraphicsItem* item : std::as_const(m_viewItems)) {
        auto* nodeItem = dynamic_cast<CanvasNodeItem*>(item);
        if (!nodeItem) {
            continue;
        }

        const QVector<QPointF> ports = nodeItem->scenePorts();
        for (int index = 0; index < ports.size(); ++index) {
            const qreal distance = QLineF(scenePos, ports[index]).length();
            if (distance >= bestDistance) {
                continue;
            }
            bestDistance = distance;
            bestHit.itemId = nodeItem->itemId();
            bestHit.portIndex = index;
            bestHit.scenePos = ports[index];
        }
    }

    return bestHit;
}

QPointF EditorSession::resolveEndpoint(const EndpointRef& endpoint) const {
    if (endpoint.isAttached()) {
        auto* view = dynamic_cast<CanvasNodeItem*>(m_viewItems.value(endpoint.itemId, nullptr));
        if (view) {
            const QVector<QPointF> ports = view->scenePorts();
            if (endpoint.portIndex >= 0 && endpoint.portIndex < ports.size()) {
                return ports[endpoint.portIndex];
            }
        }
    }
    return endpoint.position;
}

void EditorSession::insertItemsInternal(const std::vector<std::unique_ptr<DiagramItemModel>>& items) {
    m_replayingCommand = true;

    QStringList insertedIds;
    for (const auto& item : items) {
        insertedIds.push_back(item->id);
        m_document.upsert(item->clone());
    }

    for (const QString& id : insertedIds) {
        createViewForItem(id);
        syncViewForItem(id);
    }

    refreshAllConnectors();
    m_scene->clearSelection();
    for (const QString& id : insertedIds) {
        if (auto* item = m_viewItems.value(id, nullptr)) {
            item->setSelected(true);
        }
    }

    m_replayingCommand = false;
    emit documentChanged();
}

void EditorSession::removeItemsInternal(const QStringList& itemIds) {
    m_replayingCommand = true;

    for (const QString& id : itemIds) {
        removeViewForItem(id);
        m_document.take(id);
    }

    refreshAllConnectors();
    m_replayingCommand = false;
    emit documentChanged();
}

void EditorSession::replaceItemsInternal(const std::vector<std::unique_ptr<DiagramItemModel>>& items) {
    m_replayingCommand = true;

    for (const auto& item : items) {
        m_document.upsert(item->clone());
    }
    for (const auto& item : items) {
        createViewForItem(item->id);
        syncViewForItem(item->id);
    }

    refreshAllConnectors();
    m_replayingCommand = false;
    emit documentChanged();
}

void EditorSession::resetDocumentInternal(std::vector<std::unique_ptr<DiagramItemModel>> items) {
    m_scene->clearSelection();
    const QList<QString> keys = m_viewItems.keys();
    for (const QString& id : keys) {
        removeViewForItem(id);
    }

    m_document.clear();
    for (auto& item : items) {
        m_document.upsert(std::move(item));
    }
    for (const QString& id : m_document.itemIds()) {
        createViewForItem(id);
        syncViewForItem(id);
    }

    refreshAllConnectors();
    m_clipboard.clear();
    m_pendingInteraction.reset();
    m_undoStack->clear();
    m_forceDirty = false;
    emitDirtyStateIfChanged();
    emit documentChanged();
}

void EditorSession::createViewForItem(const QString& itemId) {
    if (m_viewItems.contains(itemId)) {
        return;
    }

    const auto* item = m_document.item(itemId);
    if (!item) {
        return;
    }

    QGraphicsItem* view = nullptr;
    if (item->kind() == ItemKind::Node) {
        const auto* definition = m_registry.definition(item->typeId);
        if (!definition) {
            return;
        }
        view = new CanvasNodeItem(this, definition, itemId);
    } else {
        view = new CanvasConnectorItem(this, itemId);
    }

    m_scene->addItem(view);
    m_viewItems.insert(itemId, view);
}

void EditorSession::removeViewForItem(const QString& itemId) {
    QGraphicsItem* item = m_viewItems.take(itemId);
    if (!item) {
        return;
    }
    m_scene->removeItem(item);
    delete item;
}

void EditorSession::syncViewForItem(const QString& itemId) {
    createViewForItem(itemId);
    auto* item = dynamic_cast<CanvasDiagramItem*>(m_viewItems.value(itemId, nullptr));
    if (item) {
        item->syncFromModel();
    }
}

void EditorSession::refreshConnectorsForNode(const QString& nodeId) {
    const QStringList connectorIds = m_document.connectorsForNode(nodeId);
    for (const QString& connectorId : connectorIds) {
        syncViewForItem(connectorId);
    }
}

void EditorSession::refreshAllConnectors() {
    for (const QString& id : m_document.itemIds()) {
        if (m_document.connector(id)) {
            syncViewForItem(id);
        }
    }
}

QStringList EditorSession::expandSelectionForDeletion(const QStringList& itemIds) const {
    QSet<QString> expanded(itemIds.begin(), itemIds.end());
    for (const QString& id : itemIds) {
        if (!m_document.node(id)) {
            continue;
        }
        const QStringList connectorIds = m_document.connectorsForNode(id);
        for (const QString& connectorId : connectorIds) {
            expanded.insert(connectorId);
        }
    }
    return QStringList(expanded.begin(), expanded.end());
}

void EditorSession::applyToItems(
    const QStringList& itemIds,
    const QString& description,
    const std::function<void(DiagramItemModel&)>& mutator) {
    if (itemIds.isEmpty()) {
        return;
    }

    auto before = cloneItemsWithResolvedEndpoints(itemIds);
    auto after = cloneModels(before);
    for (auto& item : after) {
        mutator(*item);
    }

    if (!vectorChanged(before, after)) {
        return;
    }

    m_undoStack->push(new UpdateItemsCommand(this, std::move(before), std::move(after), description));
}

std::vector<std::unique_ptr<DiagramItemModel>> EditorSession::cloneItemsWithResolvedEndpoints(
    const QStringList& itemIds) const {
    auto items = m_document.cloneItems(itemIds);
    for (auto& item : items) {
        if (item->kind() != ItemKind::Connector) {
            continue;
        }
        auto* connector = static_cast<ConnectorModel*>(item.get());
        connector->start.position = resolveEndpoint(connector->start);
        connector->end.position = resolveEndpoint(connector->end);
    }
    return items;
}

std::vector<std::unique_ptr<DiagramItemModel>> EditorSession::cloneAllWithResolvedEndpoints() const {
    auto items = m_document.cloneAll();
    for (auto& item : items) {
        if (item->kind() != ItemKind::Connector) {
            continue;
        }
        auto* connector = static_cast<ConnectorModel*>(item.get());
        connector->start.position = resolveEndpoint(connector->start);
        connector->end.position = resolveEndpoint(connector->end);
    }
    return items;
}

QRectF EditorSession::itemsBoundingRect(const std::vector<std::unique_ptr<DiagramItemModel>>& items) {
    QRectF bounds;
    bool first = true;

    for (const auto& item : items) {
        QRectF itemRect;
        if (item->kind() == ItemKind::Node) {
            itemRect = static_cast<const NodeModel*>(item.get())->rect;
        } else {
            const auto* connector = static_cast<const ConnectorModel*>(item.get());
            itemRect = QRectF(connector->start.position, connector->end.position).normalized();
        }

        if (first) {
            bounds = itemRect;
            first = false;
        } else {
            bounds = bounds.united(itemRect);
        }
    }

    return bounds;
}

qreal EditorSession::snapCoordinate(qreal value) const {
    if (!m_snapToGridEnabled || m_gridSize <= 1) {
        return value;
    }
    return std::round(value / static_cast<qreal>(m_gridSize)) * static_cast<qreal>(m_gridSize);
}

void EditorSession::emitDirtyStateIfChanged() {
    const bool dirty = isDirty();
    if (dirty == m_lastDirtyState) {
        return;
    }
    m_lastDirtyState = dirty;
    emit dirtyChanged(dirty);
}

}  // namespace flowchart
