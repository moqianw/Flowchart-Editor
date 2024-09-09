#pragma once
#include <QLabel>

#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QCheckBox>
#include <QWidgetAction>
#include <QRubberBand>
#include "base.h"
#include "OperationRecord.h"
#include <QRect>
#include <QFileDialog>
#include <QtSvg>

namespace base {

    class SelectionRect : public QGraphicsRectItem {
    public:
        SelectionRect(QGraphicsItem* parent = nullptr) : QGraphicsRectItem(parent) {}

    protected:
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override {
            Q_UNUSED(option);
            Q_UNUSED(widget);

            painter->setPen(QPen(Qt::DashLine));
            painter->setBrush(QBrush(QColor(0, 0, 255, 50))); // 半透明蓝色填充
            painter->drawRect(rect());
        }
    };
class GraphicsView :public QGraphicsView{

    bool m_drawGrid = true;//绘制网格线
public:
    QMainWindow* main = nullptr;
    QDrag* drag = nullptr;
    QMimeData* mimeData = nullptr;

    GraphicsView(QGraphicsScene* scene, QWidget* parent);
    SelectionRect* selectionRect = nullptr;
    qreal rotationAngle = 0;//旋转角度
    QGraphicsScene* scene;

protected:


    QPointF startPos;
    QRubberBand* rubberBand = nullptr;
    QPoint lastPost;
    void dragEnterEvent(QDragEnterEvent* event) override;

    void dragMoveEvent(QDragMoveEvent* event) override;

    void dropEvent(QDropEvent* event) override;

    void drawBackground(QPainter* painter, const QRectF& rect) override;

    void contextMenuEvent(QContextMenuEvent* event) override;

    //选择：
    bool choose = false;
    void mousePressEvent(QMouseEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;


public slots:
    void toSvg(QString filename);

    void toPng(QString filename);


    //旋转缩放
    void zoomOut();
    void zoomIn();
    void rotateLeft();
    void rotateright();
    void setRotation(qreal angle);


    //缩放/旋转
    void scaleSelectedItem(qreal factor);
    void rotateSelectedItem(qreal angleDelta);
};


}
