#include "canvasitems.h"

#include "editorsession.h"
#include "shaperegistry.h"

#include <QFocusEvent>
#include <QLineF>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include <algorithm>
#include <cmath>
#include <optional>

namespace flowchart {

namespace {

constexpr qreal kHandleSize = 8.0;
constexpr qreal kHandleMargin = 10.0;
constexpr qreal kMinNodeSize = 32.0;
constexpr qreal kPi = 3.14159265358979323846;

QRectF centeredRect(const QPointF& center, qreal size) {
    return QRectF(center.x() - size / 2.0, center.y() - size / 2.0, size, size);
}

void appendSegmentPoint(QVector<QPointF>& segments, const QPointF& point) {
    if (!segments.isEmpty() && QLineF(segments.back(), point).length() < 0.5) {
        return;
    }
    segments.push_back(point);
}

QPainterPath arrowPath(
    const QPointF& start,
    const QPointF& end,
    qreal penWidth,
    const std::optional<qreal>& bendX = std::nullopt,
    const std::optional<qreal>& bendY = std::nullopt) {
    QPainterPath path;
    QVector<QPointF> segments;
    segments.push_back(start);

    const qreal dx = end.x() - start.x();
    const qreal dy = end.y() - start.y();
    if (bendX.has_value()) {
        appendSegmentPoint(segments, QPointF(*bendX, start.y()));
        appendSegmentPoint(segments, QPointF(*bendX, end.y()));
        appendSegmentPoint(segments, end);
    } else if (bendY.has_value()) {
        appendSegmentPoint(segments, QPointF(start.x(), *bendY));
        appendSegmentPoint(segments, QPointF(end.x(), *bendY));
        appendSegmentPoint(segments, end);
    } else if (std::abs(dx) < 1.0 || std::abs(dy) < 1.0) {
        appendSegmentPoint(segments, end);
    } else {
        const qreal midX = start.x() + dx / 2.0;
        appendSegmentPoint(segments, QPointF(midX, start.y()));
        appendSegmentPoint(segments, QPointF(midX, end.y()));
        appendSegmentPoint(segments, end);
    }

    path.moveTo(segments.front());
    for (int index = 1; index < segments.size(); ++index) {
        path.lineTo(segments[index]);
    }

    const QPointF arrowBase = segments.size() >= 2 ? segments[segments.size() - 2] : start;
    const qreal angle = std::atan2(end.y() - arrowBase.y(), end.x() - arrowBase.x());
    const qreal arrowSize = std::max<qreal>(penWidth * 3.0, 8.0);
    const QPointF arrowP1 = end - QPointF(
        arrowSize * std::cos(angle + kPi / 6.0),
        arrowSize * std::sin(angle + kPi / 6.0));
    const QPointF arrowP2 = end - QPointF(
        arrowSize * std::cos(angle - kPi / 6.0),
        arrowSize * std::sin(angle - kPi / 6.0));

    path.moveTo(end);
    path.lineTo(arrowP1);
    path.lineTo(arrowP2);
    path.closeSubpath();
    return path;
}

}  // namespace

CanvasNodeItem::EditableTextItem::EditableTextItem(QGraphicsItem* parent)
    : QGraphicsTextItem(parent) {}

void CanvasNodeItem::EditableTextItem::beginEditing() {
    m_initialText = toPlainText();
    setTextInteractionFlags(Qt::TextEditorInteraction);
    setFocus(Qt::MouseFocusReason);
}

void CanvasNodeItem::EditableTextItem::setCommitHandler(std::function<void(const QString&)> handler) {
    m_commitHandler = std::move(handler);
}

void CanvasNodeItem::EditableTextItem::focusOutEvent(QFocusEvent* event) {
    QGraphicsTextItem::focusOutEvent(event);
    setTextInteractionFlags(Qt::NoTextInteraction);
    if (m_commitHandler && m_initialText != toPlainText()) {
        m_commitHandler(toPlainText());
    }
}

CanvasNodeItem::CanvasNodeItem(EditorSession* session, const ShapeDefinition* definition, const QString& itemId)
    : m_session(session)
    , m_definition(definition)
    , m_itemId(itemId)
    , m_textItem(new EditableTextItem(this)) {
    setFlag(ItemIsSelectable);
    setFlag(ItemIsMovable);
    setFlag(ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);

    m_textItem->setTextInteractionFlags(Qt::NoTextInteraction);
    m_textItem->setCommitHandler([this](const QString& text) {
        m_session->setText(QStringList{m_itemId}, text);
    });
}

QString CanvasNodeItem::itemId() const {
    return m_itemId;
}

void CanvasNodeItem::syncFromModel() {
    const auto* model = m_session->document().node(m_itemId);
    if (!model) {
        return;
    }

    m_syncing = true;
    prepareGeometryChange();

    m_localRect = QRectF(0.0, 0.0, model->rect.width(), model->rect.height());
    setPos(model->rect.topLeft());
    setZValue(model->zValue);
    setTransformOriginPoint(m_localRect.center());
    setRotation(model->style.rotation);
    setScale(model->style.scale);

    m_path = m_definition->buildPath(m_localRect, model->props);

    QFont font = model->style.font;
    font.setBold(model->style.bold);
    font.setItalic(model->style.italic);
    font.setUnderline(model->style.underline);
    m_textItem->setFont(font);
    m_textItem->setPlainText(model->text);
    updateTextLayout();

    m_syncing = false;
    update();
}

QVector<QPointF> CanvasNodeItem::scenePorts() const {
    QVector<QPointF> ports;
    const auto* model = m_session->document().node(m_itemId);
    if (!model) {
        return ports;
    }

    const QVector<QPointF> localPorts = m_definition->buildPorts(m_localRect, model->props);
    ports.reserve(localPorts.size());
    for (const QPointF& port : localPorts) {
        ports.push_back(mapToScene(port));
    }
    return ports;
}

QRectF CanvasNodeItem::boundingRect() const {
    return m_localRect.adjusted(-kHandleMargin, -kHandleMargin, kHandleMargin, kHandleMargin);
}

void CanvasNodeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    const auto* model = m_session->document().node(m_itemId);
    if (!model) {
        return;
    }

    QPen pen(model->style.strokeColor);
    pen.setWidth(model->style.strokeWidth);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->setBrush(model->style.fillColor);
    painter->drawPath(m_path);

    if (!isSelected()) {
        return;
    }

    QPen selectionPen(QColor(55, 126, 255));
    selectionPen.setStyle(Qt::DashLine);
    selectionPen.setWidth(1);
    selectionPen.setCosmetic(true);
    painter->setPen(selectionPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(m_localRect);

    painter->setBrush(QColor(255, 255, 255));
    for (const QRectF& handle : handleRects()) {
        painter->drawRect(handle);
    }
}

QVariant CanvasNodeItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (!m_syncing && change == ItemPositionChange) {
        return m_session->snapPoint(value.toPointF());
    }

    if (!m_syncing && change == ItemPositionHasChanged && m_session->isInteractiveChangeActiveFor(m_itemId)) {
        const auto* model = m_session->document().node(m_itemId);
        if (model) {
            m_session->applyNodeGeometryInteractive(
                m_itemId,
                QRectF(pos(), QSizeF(model->rect.width(), model->rect.height())),
                rotation(),
                scale());
        }
    }

    return QGraphicsObject::itemChange(change, value);
}

void CanvasNodeItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    m_pressCommitted = false;
    if (event->button() != Qt::LeftButton) {
        QGraphicsObject::mousePressEvent(event);
        return;
    }

    m_activeHandle = handleAt(event->pos());
    if (m_activeHandle != ResizeHandle::None) {
        if (!isSelected()) {
            scene()->clearSelection();
            setSelected(true);
        }
        m_resizeStartScenePos = event->scenePos();
        m_resizeStartRect = currentSceneRect();
        m_session->beginInteractiveChange(QStringList{m_itemId}, QStringLiteral("Resize item"));
        m_pressCommitted = true;
        event->accept();
        return;
    }

    if (!isSelected()) {
        scene()->clearSelection();
        setSelected(true);
    }
    m_session->beginInteractiveChange(m_session->selectedItemIds(), QStringLiteral("Move items"));
    QGraphicsObject::mousePressEvent(event);
}

void CanvasNodeItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_activeHandle == ResizeHandle::None) {
        QGraphicsObject::mouseMoveEvent(event);
        return;
    }

    QRectF rect = m_resizeStartRect;
    const QPointF delta = event->scenePos() - m_resizeStartScenePos;

    if (m_activeHandle == ResizeHandle::Left
        || m_activeHandle == ResizeHandle::TopLeft
        || m_activeHandle == ResizeHandle::BottomLeft) {
        rect.setLeft(rect.left() + delta.x());
    }
    if (m_activeHandle == ResizeHandle::Right
        || m_activeHandle == ResizeHandle::TopRight
        || m_activeHandle == ResizeHandle::BottomRight) {
        rect.setRight(rect.right() + delta.x());
    }
    if (m_activeHandle == ResizeHandle::Top
        || m_activeHandle == ResizeHandle::TopLeft
        || m_activeHandle == ResizeHandle::TopRight) {
        rect.setTop(rect.top() + delta.y());
    }
    if (m_activeHandle == ResizeHandle::Bottom
        || m_activeHandle == ResizeHandle::BottomLeft
        || m_activeHandle == ResizeHandle::BottomRight) {
        rect.setBottom(rect.bottom() + delta.y());
    }

    if (rect.width() < kMinNodeSize) {
        if (m_activeHandle == ResizeHandle::Left
            || m_activeHandle == ResizeHandle::TopLeft
            || m_activeHandle == ResizeHandle::BottomLeft) {
            rect.setLeft(rect.right() - kMinNodeSize);
        } else {
            rect.setRight(rect.left() + kMinNodeSize);
        }
    }
    if (rect.height() < kMinNodeSize) {
        if (m_activeHandle == ResizeHandle::Top
            || m_activeHandle == ResizeHandle::TopLeft
            || m_activeHandle == ResizeHandle::TopRight) {
            rect.setTop(rect.bottom() - kMinNodeSize);
        } else {
            rect.setBottom(rect.top() + kMinNodeSize);
        }
    }

    m_session->applyNodeGeometryInteractive(m_itemId, rect.normalized(), rotation(), scale());
    event->accept();
}

void CanvasNodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (m_activeHandle != ResizeHandle::None) {
        m_activeHandle = ResizeHandle::None;
        if (!m_pressCommitted) {
            m_session->commitInteractiveChange();
        } else {
            m_session->commitInteractiveChange();
            m_pressCommitted = false;
        }
        event->accept();
        return;
    }

    QGraphicsObject::mouseReleaseEvent(event);
    m_session->commitInteractiveChange();
}

void CanvasNodeItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    Q_UNUSED(event);
    m_textItem->beginEditing();
}

void CanvasNodeItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    updateCursor(event->pos());
    QGraphicsObject::hoverMoveEvent(event);
}

void CanvasNodeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    unsetCursor();
    QGraphicsObject::hoverLeaveEvent(event);
}

CanvasNodeItem::ResizeHandle CanvasNodeItem::handleAt(const QPointF& pos) const {
    if (!isSelected()) {
        return ResizeHandle::None;
    }

    const QVector<QRectF> rects = handleRects();
    for (int index = 0; index < rects.size(); ++index) {
        if (!rects[index].contains(pos)) {
            continue;
        }
        switch (index) {
        case 0:
            return ResizeHandle::TopLeft;
        case 1:
            return ResizeHandle::Top;
        case 2:
            return ResizeHandle::TopRight;
        case 3:
            return ResizeHandle::Right;
        case 4:
            return ResizeHandle::BottomRight;
        case 5:
            return ResizeHandle::Bottom;
        case 6:
            return ResizeHandle::BottomLeft;
        case 7:
            return ResizeHandle::Left;
        default:
            break;
        }
    }
    return ResizeHandle::None;
}

QVector<QRectF> CanvasNodeItem::handleRects() const {
    return {
        centeredRect(m_localRect.topLeft(), kHandleSize),
        centeredRect(QPointF(m_localRect.center().x(), m_localRect.top()), kHandleSize),
        centeredRect(m_localRect.topRight(), kHandleSize),
        centeredRect(QPointF(m_localRect.right(), m_localRect.center().y()), kHandleSize),
        centeredRect(m_localRect.bottomRight(), kHandleSize),
        centeredRect(QPointF(m_localRect.center().x(), m_localRect.bottom()), kHandleSize),
        centeredRect(m_localRect.bottomLeft(), kHandleSize),
        centeredRect(QPointF(m_localRect.left(), m_localRect.center().y()), kHandleSize),
    };
}

QRectF CanvasNodeItem::currentSceneRect() const {
    const auto* model = m_session->document().node(m_itemId);
    return model ? model->rect : QRectF();
}

void CanvasNodeItem::updateTextLayout() {
    const qreal textWidth = std::max<qreal>(m_localRect.width() - 12.0, 24.0);
    m_textItem->setTextWidth(textWidth);
    const QRectF textRect = m_textItem->boundingRect();
    const QPointF textPos(
        m_localRect.left() + 6.0,
        m_localRect.center().y() - textRect.height() / 2.0);
    m_textItem->setPos(textPos);
}

void CanvasNodeItem::updateCursor(const QPointF& pos) {
    switch (handleAt(pos)) {
    case ResizeHandle::TopLeft:
    case ResizeHandle::BottomRight:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case ResizeHandle::TopRight:
    case ResizeHandle::BottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case ResizeHandle::Left:
    case ResizeHandle::Right:
        setCursor(Qt::SizeHorCursor);
        break;
    case ResizeHandle::Top:
    case ResizeHandle::Bottom:
        setCursor(Qt::SizeVerCursor);
        break;
    case ResizeHandle::None:
        unsetCursor();
        break;
    }
}

CanvasConnectorItem::CanvasConnectorItem(EditorSession* session, const QString& itemId)
    : m_session(session)
    , m_itemId(itemId) {
    setFlag(ItemIsSelectable);
    setZValue(1.0);
}

QString CanvasConnectorItem::itemId() const {
    return m_itemId;
}

void CanvasConnectorItem::syncFromModel() {
    const auto* model = m_session->document().connector(m_itemId);
    if (!model) {
        return;
    }

    m_syncing = true;
    setZValue(model->zValue);
    m_startPoint = m_session->resolveEndpoint(model->start);
    m_endPoint = m_session->resolveEndpoint(model->end);
    rebuildPath();
    m_syncing = false;
    update();
}

QRectF CanvasConnectorItem::boundingRect() const {
    return m_path.boundingRect().adjusted(-16.0, -16.0, 16.0, 16.0);
}

void CanvasConnectorItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    const auto* model = m_session->document().connector(m_itemId);
    if (!model) {
        return;
    }

    QPen pen(model->style.strokeColor);
    pen.setWidth(model->style.strokeWidth);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->setBrush(model->style.fillColor);
    painter->drawPath(m_path);

    if (!isSelected()) {
        return;
    }

    painter->setBrush(QColor(255, 255, 255));
    painter->drawEllipse(handleRect(m_startPoint));
    painter->drawEllipse(handleRect(m_endPoint));
    if (m_hasBendHandle) {
        painter->setBrush(QColor(55, 126, 255));
        painter->drawEllipse(handleRect(m_bendHandlePoint));
    }
}

void CanvasConnectorItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() != Qt::LeftButton) {
        QGraphicsObject::mousePressEvent(event);
        return;
    }

    if (handleRect(m_startPoint).contains(event->pos())) {
        m_activeHandle = EndpointHandle::Start;
    } else if (handleRect(m_endPoint).contains(event->pos())) {
        m_activeHandle = EndpointHandle::End;
    } else if (m_hasBendHandle && handleRect(m_bendHandlePoint).contains(event->pos())) {
        m_activeHandle = EndpointHandle::Bend;
    }

    if (m_activeHandle == EndpointHandle::None) {
        QGraphicsObject::mousePressEvent(event);
        return;
    }

    if (!isSelected()) {
        scene()->clearSelection();
        setSelected(true);
    }

    m_session->beginInteractiveChange(
        QStringList{m_itemId},
        m_activeHandle == EndpointHandle::Bend ? QStringLiteral("Adjust line bend") : QStringLiteral("Reconnect line"));
    event->accept();
}

void CanvasConnectorItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_activeHandle == EndpointHandle::None) {
        QGraphicsObject::mouseMoveEvent(event);
        return;
    }

    if (m_activeHandle == EndpointHandle::Bend) {
        const auto* model = m_session->document().connector(m_itemId);
        if (!model) {
            return;
        }
        QJsonObject props = model->props;
        props.remove(QStringLiteral("bendX"));
        props.remove(QStringLiteral("bendY"));
        if (m_bendAdjustsX) {
            props.insert(QStringLiteral("bendX"), event->scenePos().x());
        } else {
            props.insert(QStringLiteral("bendY"), event->scenePos().y());
        }
        m_session->applyConnectorBendInteractive(m_itemId, props);
        event->accept();
        return;
    }

    const auto* model = m_session->document().connector(m_itemId);
    if (!model) {
        return;
    }

    EndpointRef start = model->start;
    EndpointRef end = model->end;
    PortHit hit = m_session->hitTestPort(event->scenePos(), 16.0);

    EndpointRef endpoint;
    endpoint.position = event->scenePos();
    if (hit.isValid()) {
        endpoint.itemId = hit.itemId;
        endpoint.portIndex = hit.portIndex;
        endpoint.position = hit.scenePos;
    }

    if (m_activeHandle == EndpointHandle::Start) {
        start = endpoint;
    } else {
        end = endpoint;
    }

    m_session->applyConnectorInteractive(m_itemId, start, end);
    event->accept();
}

void CanvasConnectorItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (m_activeHandle == EndpointHandle::None) {
        QGraphicsObject::mouseReleaseEvent(event);
        return;
    }

    m_activeHandle = EndpointHandle::None;
    m_session->commitInteractiveChange();
    event->accept();
}

QRectF CanvasConnectorItem::handleRect(const QPointF& point) const {
    return centeredRect(point, 10.0);
}

void CanvasConnectorItem::rebuildPath() {
    prepareGeometryChange();
    const auto* model = m_session->document().connector(m_itemId);
    if (!model) {
        return;
    }

    std::optional<qreal> bendX;
    std::optional<qreal> bendY;

    if (model->props.contains(QStringLiteral("bendX"))) {
        bendX = model->props.value(QStringLiteral("bendX")).toDouble();
    }
    if (model->props.contains(QStringLiteral("bendY"))) {
        bendY = model->props.value(QStringLiteral("bendY")).toDouble();
    }

    const qreal dx = std::abs(m_endPoint.x() - m_startPoint.x());
    const qreal dy = std::abs(m_endPoint.y() - m_startPoint.y());
    m_hasBendHandle = QLineF(m_startPoint, m_endPoint).length() >= 1.0;

    if (bendX.has_value()) {
        m_bendAdjustsX = true;
        m_bendHandlePoint = QPointF(*bendX, (m_startPoint.y() + m_endPoint.y()) / 2.0);
    } else if (bendY.has_value()) {
        m_bendAdjustsX = false;
        m_bendHandlePoint = QPointF((m_startPoint.x() + m_endPoint.x()) / 2.0, *bendY);
    } else if (dy < 1.0) {
        m_bendAdjustsX = false;
        m_bendHandlePoint = QPointF((m_startPoint.x() + m_endPoint.x()) / 2.0, m_startPoint.y());
    } else {
        m_bendAdjustsX = true;
        const qreal defaultBendX = dx < 1.0
            ? m_startPoint.x()
            : m_startPoint.x() + (m_endPoint.x() - m_startPoint.x()) / 2.0;
        if (dx >= 1.0 && dy >= 1.0) {
            bendX = defaultBendX;
        }
        m_bendHandlePoint = QPointF(defaultBendX, (m_startPoint.y() + m_endPoint.y()) / 2.0);
    }

    if (!m_hasBendHandle) {
        m_bendHandlePoint = QPointF();
    }
    m_path = arrowPath(m_startPoint, m_endPoint, model->style.strokeWidth, bendX, bendY);
}

}  // namespace flowchart
