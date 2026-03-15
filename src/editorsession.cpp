#include "editorsession.h"

#include "canvasitems.h"
#include "commands.h"
#include "documentserializer.h"

#include <QGraphicsItem>
#include <QSet>

#include <algorithm>
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
    connect(m_undoStack, &QUndoStack::cleanChanged, this, [this](bool clean) {
        emit dirtyChanged(!clean);
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

bool EditorSession::isDirty() const {
    return !m_undoStack->isClean();
}

void EditorSession::markClean() {
    m_undoStack->setClean();
    emit dirtyChanged(false);
}

void EditorSession::resetDocument() {
    resetDocumentInternal({});
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

void EditorSession::createItem(const QString& typeId, const QPointF& scenePos) {
    const ShapeDefinition* definition = m_registry.definition(typeId);
    if (!definition) {
        return;
    }

    std::vector<std::unique_ptr<DiagramItemModel>> items;
    items.reserve(1);

    if (definition->connector) {
        auto connector = std::make_unique<ConnectorModel>();
        connector->id = createItemId();
        connector->typeId = definition->typeId;
        connector->style = ItemStyle{};
        if (definition->initializeStyle) {
            definition->initializeStyle(connector->style);
        }
        connector->start.position = scenePos;
        connector->end.position = scenePos + QPointF(definition->defaultSize.width(), definition->defaultSize.height());
        items.push_back(std::move(connector));
    } else {
        auto node = std::make_unique<NodeModel>();
        node->id = createItemId();
        node->typeId = definition->typeId;
        node->rect = QRectF(scenePos, definition->defaultSize);
        node->props = definition->defaultProps;
        node->style = ItemStyle{};
        if (definition->initializeStyle) {
            definition->initializeStyle(node->style);
        }
        items.push_back(std::move(node));
    }

    m_undoStack->push(new AddItemsCommand(this, std::move(items), QStringLiteral("Create item")));
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
    updated->rect = rect.normalized();
    updated->style.rotation = rotation;
    updated->style.scale = scale;
    m_document.upsert(std::move(updated));
    syncViewForItem(itemId);
    refreshConnectorsForNode(itemId);
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
    m_document.upsert(std::move(updated));
    syncViewForItem(itemId);
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
}

void EditorSession::removeItemsInternal(const QStringList& itemIds) {
    m_replayingCommand = true;

    for (const QString& id : itemIds) {
        removeViewForItem(id);
        m_document.take(id);
    }

    refreshAllConnectors();
    m_replayingCommand = false;
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
    m_undoStack->setClean();
    emit dirtyChanged(false);
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

}  // namespace flowchart
