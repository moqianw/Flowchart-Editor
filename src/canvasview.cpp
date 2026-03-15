#include "canvasview.h"

#include "editorsession.h"

#include <QCheckBox>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QSvgGenerator>
#include <QWidgetAction>

#include <cmath>

namespace flowchart {

namespace {

bool hasPaletteMime(const QMimeData* mimeData) {
    return mimeData && mimeData->hasFormat("application/x-flowchart-type");
}

QString paletteTypeId(const QMimeData* mimeData) {
    return QString::fromUtf8(mimeData->data("application/x-flowchart-type"));
}

}  // namespace

CanvasView::CanvasView(EditorSession* session, QWidget* parent)
    : QGraphicsView(session->scene(), parent)
    , m_session(session) {
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::RubberBandDrag);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QPointF CanvasView::sceneCenter() const {
    return mapToScene(viewport()->rect().center());
}

void CanvasView::exportToPng(const QString& fileName) const {
    const QRectF bounds = scene()->itemsBoundingRect().isEmpty() ? sceneRect() : scene()->itemsBoundingRect();
    QPixmap pixmap(bounds.size().toSize() + QSize(24, 24));
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);
    scene()->render(&painter, QRectF(), bounds.adjusted(-12, -12, 12, 12));
    pixmap.save(fileName);
}

void CanvasView::exportToSvg(const QString& fileName) const {
    const QRectF bounds = scene()->itemsBoundingRect().isEmpty() ? sceneRect() : scene()->itemsBoundingRect();

    QSvgGenerator generator;
    generator.setFileName(fileName);
    generator.setViewBox(bounds.adjusted(-12, -12, 12, 12));
    generator.setSize(bounds.size().toSize() + QSize(24, 24));

    QPainter painter;
    if (!painter.begin(&generator)) {
        return;
    }

    scene()->render(&painter, QRectF(), bounds.adjusted(-12, -12, 12, 12));
}

void CanvasView::zoomInView() {
    scale(1.2, 1.2);
}

void CanvasView::zoomOutView() {
    scale(1.0 / 1.2, 1.0 / 1.2);
}

void CanvasView::rotateViewLeft() {
    m_rotation -= 90.0;
    setTransform(QTransform().rotate(m_rotation));
}

void CanvasView::rotateViewRight() {
    m_rotation += 90.0;
    setTransform(QTransform().rotate(m_rotation));
}

void CanvasView::drawBackground(QPainter* painter, const QRectF& rect) {
    QGraphicsView::drawBackground(painter, rect);

    if (!m_drawGrid) {
        return;
    }

    painter->setPen(QPen(QColor(220, 220, 220), 0));
    constexpr int gridSize = 20;

    const int left = static_cast<int>(std::floor(rect.left() / gridSize) * gridSize);
    const int top = static_cast<int>(std::floor(rect.top() / gridSize) * gridSize);

    for (int x = left; x < rect.right(); x += gridSize) {
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }
    for (int y = top; y < rect.bottom(); y += gridSize) {
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }
}

void CanvasView::dragEnterEvent(QDragEnterEvent* event) {
    if (hasPaletteMime(event->mimeData())) {
        event->acceptProposedAction();
        return;
    }
    QGraphicsView::dragEnterEvent(event);
}

void CanvasView::dragMoveEvent(QDragMoveEvent* event) {
    if (hasPaletteMime(event->mimeData())) {
        event->acceptProposedAction();
        return;
    }
    QGraphicsView::dragMoveEvent(event);
}

void CanvasView::dropEvent(QDropEvent* event) {
    if (hasPaletteMime(event->mimeData())) {
        const QString typeId = paletteTypeId(event->mimeData());
        m_session->createItem(typeId, mapToScene(event->position().toPoint()));
        event->acceptProposedAction();
        return;
    }
    QGraphicsView::dropEvent(event);
}

void CanvasView::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    auto* checkBox = new QCheckBox(QStringLiteral("显示网格线"), &menu);
    checkBox->setChecked(m_drawGrid);
    connect(checkBox, &QCheckBox::toggled, this, [this](bool checked) {
        m_drawGrid = checked;
        viewport()->update();
    });

    auto* action = new QWidgetAction(&menu);
    action->setDefaultWidget(checkBox);
    menu.addAction(action);
    menu.exec(event->globalPos());
}

bool CanvasView::viewportEvent(QEvent* event) {
    switch (event->type()) {
    case QEvent::DragEnter:
        dragEnterEvent(static_cast<QDragEnterEvent*>(event));
        return event->isAccepted();
    case QEvent::DragMove:
        dragMoveEvent(static_cast<QDragMoveEvent*>(event));
        return event->isAccepted();
    case QEvent::Drop:
        dropEvent(static_cast<QDropEvent*>(event));
        return event->isAccepted();
    default:
        break;
    }

    return QGraphicsView::viewportEvent(event);
}

}  // namespace flowchart
