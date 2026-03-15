#pragma once

#include "diagramtypes.h"

#include <QGraphicsObject>
#include <QGraphicsTextItem>
#include <QPainterPath>

#include <functional>

namespace flowchart {

class EditorSession;
struct ShapeDefinition;

class CanvasDiagramItem {
public:
    virtual ~CanvasDiagramItem() = default;

    virtual QString itemId() const = 0;
    virtual void syncFromModel() = 0;
};

class CanvasNodeItem final : public QGraphicsObject, public CanvasDiagramItem {
    Q_OBJECT

public:
    CanvasNodeItem(EditorSession* session, const ShapeDefinition* definition, const QString& itemId);

    QString itemId() const override;
    void syncFromModel() override;
    QVector<QPointF> scenePorts() const;

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    enum class ResizeHandle {
        None,
        Left,
        Right,
        Top,
        Bottom,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
    };

    class EditableTextItem final : public QGraphicsTextItem {
    public:
        explicit EditableTextItem(QGraphicsItem* parent = nullptr);

        void beginEditing();
        void setCommitHandler(std::function<void(const QString&)> handler);

    protected:
        void focusOutEvent(QFocusEvent* event) override;

    private:
        QString m_initialText;
        std::function<void(const QString&)> m_commitHandler;
    };

    ResizeHandle handleAt(const QPointF& pos) const;
    QVector<QRectF> handleRects() const;
    QRectF currentSceneRect() const;
    void updateTextLayout();
    void updateCursor(const QPointF& pos);

    EditorSession* m_session;
    const ShapeDefinition* m_definition;
    QString m_itemId;
    QRectF m_localRect;
    QPainterPath m_path;
    EditableTextItem* m_textItem = nullptr;
    bool m_syncing = false;
    bool m_pressCommitted = false;
    ResizeHandle m_activeHandle = ResizeHandle::None;
    QPointF m_resizeStartScenePos;
    QRectF m_resizeStartRect;
};

class CanvasConnectorItem final : public QGraphicsObject, public CanvasDiagramItem {
    Q_OBJECT

public:
    CanvasConnectorItem(EditorSession* session, const QString& itemId);

    QString itemId() const override;
    void syncFromModel() override;

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    enum class EndpointHandle {
        None,
        Start,
        End,
        Bend,
    };

    QRectF handleRect(const QPointF& point) const;
    void rebuildPath();

    EditorSession* m_session;
    QString m_itemId;
    QPointF m_startPoint;
    QPointF m_endPoint;
    QPointF m_bendHandlePoint;
    bool m_hasBendHandle = false;
    bool m_bendAdjustsX = true;
    QPainterPath m_path;
    bool m_syncing = false;
    EndpointHandle m_activeHandle = EndpointHandle::None;
};

}  // namespace flowchart
