#pragma once

#include <QEvent>
#include <QGraphicsView>

namespace flowchart {

class EditorSession;

class CanvasView final : public QGraphicsView {
    Q_OBJECT

public:
    CanvasView(EditorSession* session, QWidget* parent = nullptr);

    QPointF sceneCenter() const;
    void exportToPng(const QString& fileName) const;
    void exportToSvg(const QString& fileName) const;
    void zoomInView();
    void zoomOutView();
    void rotateViewLeft();
    void rotateViewRight();

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    bool viewportEvent(QEvent* event) override;

private:
    EditorSession* m_session;
    bool m_drawGrid = true;
    qreal m_rotation = 0.0;
};

}  // namespace flowchart
