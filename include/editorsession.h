#pragma once

#include "diagramdocument.h"
#include "shaperegistry.h"

#include <QGraphicsScene>
#include <QObject>
#include <QStringList>
#include <QUndoStack>

#include <functional>
#include <optional>

namespace flowchart {

class CanvasConnectorItem;
class CanvasNodeItem;

struct PortHit {
    QString itemId;
    int portIndex = -1;
    QPointF scenePos;

    bool isValid() const;
};

class EditorSession final : public QObject {
    Q_OBJECT

public:
    explicit EditorSession(QObject* parent = nullptr);

    QGraphicsScene* scene() const;
    QUndoStack* undoStack() const;
    const ShapeRegistry& registry() const;
    const DiagramDocument& document() const;
    std::vector<std::unique_ptr<DiagramItemModel>> exportDocumentSnapshot() const;

    QStringList selectedItemIds() const;
    void selectAll();
    bool hasSingleSelection() const;
    QString primarySelectedItemId() const;

    bool isDirty() const;
    void markClean();
    void markDirty();

    void resetDocument();
    void restoreRecoveredDocument(std::vector<std::unique_ptr<DiagramItemModel>> items);
    void applyRemoteSnapshot(std::vector<std::unique_ptr<DiagramItemModel>> items);
    bool loadFromFile(const QString& fileName, QString* errorMessage);
    bool saveToFile(const QString& fileName, QString* errorMessage) const;
    bool reloadShapeRegistry(QStringList* warnings = nullptr);

    void createItem(const QString& typeId, const QPointF& scenePos);
    void createConnector(const QPointF& scenePos);
    void resetSelectedConnectorBends();
    void routeSelectedConnectorsVertical();
    void routeSelectedConnectorsHorizontal();
    void setSelectedConnectorHeadStyle(const QString& headStyle);
    void setSelectedConnectorLineStyle(const QString& lineStyle);
    void deleteSelected();
    void copySelected();
    void cutSelected();
    void paste(const QPointF& scenePos);

    void applyStrokeColor(const QColor& color);
    void applyFillColor(const QColor& color);
    void applyFillAlpha(int alpha);
    void applyStrokeWidth(int width);
    void applyFontFamily(const QFont& font);
    void applyFontSize(int pointSize);
    void toggleBold();
    void toggleItalic();
    void toggleUnderline();
    void rotateSelected(qreal deltaDegrees);
    void scaleSelected(qreal factor);
    void bringForward();
    void sendBackward();
    void setText(const QStringList& itemIds, const QString& text);
    void updateNodeGeometry(
        const QString& itemId,
        const QRectF& rect,
        qreal rotation,
        qreal scale);
    void autoLayoutSelection();
    QStringList validateDocument() const;

    bool snapToGridEnabled() const;
    void setSnapToGridEnabled(bool enabled);
    int gridSize() const;
    QPointF snapPoint(const QPointF& point) const;
    QRectF snapRect(const QRectF& rect) const;

    void beginInteractiveChange(const QStringList& itemIds, const QString& description);
    bool isInteractiveChangeActiveFor(const QString& itemId) const;
    void commitInteractiveChange();

    void applyNodeGeometryInteractive(
        const QString& itemId,
        const QRectF& rect,
        qreal rotation,
        qreal scale);
    void applyConnectorInteractive(
        const QString& itemId,
        const EndpointRef& start,
        const EndpointRef& end);
    void applyConnectorBendInteractive(const QString& itemId, const QJsonObject& props);

    PortHit hitTestPort(const QPointF& scenePos, qreal threshold) const;
    QPointF resolveEndpoint(const EndpointRef& endpoint) const;

signals:
    void selectionChanged();
    void dirtyChanged(bool dirty);
    void documentChanged();

private:
    friend class AddItemsCommand;
    friend class RemoveItemsCommand;
    friend class UpdateItemsCommand;

    struct PendingInteraction {
        QString description;
        QStringList itemIds;
        std::vector<std::unique_ptr<DiagramItemModel>> before;
    };

    void insertItemsInternal(const std::vector<std::unique_ptr<DiagramItemModel>>& items);
    void removeItemsInternal(const QStringList& itemIds);
    void replaceItemsInternal(const std::vector<std::unique_ptr<DiagramItemModel>>& items);

    void resetDocumentInternal(std::vector<std::unique_ptr<DiagramItemModel>> items);
    void createViewForItem(const QString& itemId);
    void removeViewForItem(const QString& itemId);
    void syncViewForItem(const QString& itemId);
    void refreshConnectorsForNode(const QString& nodeId);
    void refreshAllConnectors();

    QStringList expandSelectionForDeletion(const QStringList& itemIds) const;
    void applyToItems(
        const QStringList& itemIds,
        const QString& description,
        const std::function<void(DiagramItemModel&)>& mutator);

    std::vector<std::unique_ptr<DiagramItemModel>> cloneItemsWithResolvedEndpoints(
        const QStringList& itemIds) const;
    std::vector<std::unique_ptr<DiagramItemModel>> cloneAllWithResolvedEndpoints() const;

    static QRectF itemsBoundingRect(const std::vector<std::unique_ptr<DiagramItemModel>>& items);
    qreal snapCoordinate(qreal value) const;
    void emitDirtyStateIfChanged();

    ShapeRegistry m_registry;
    DiagramDocument m_document;
    QGraphicsScene* m_scene;
    QUndoStack* m_undoStack;
    QHash<QString, QGraphicsItem*> m_viewItems;
    std::vector<std::unique_ptr<DiagramItemModel>> m_clipboard;
    std::optional<PendingInteraction> m_pendingInteraction;
    bool m_replayingCommand = false;
    bool m_snapToGridEnabled = true;
    int m_gridSize = 20;
    bool m_forceDirty = false;
    bool m_lastDirtyState = false;
};

}  // namespace flowchart
