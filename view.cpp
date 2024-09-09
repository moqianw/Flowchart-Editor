#include "view.h"
namespace base {
GraphicsView::GraphicsView(QGraphicsScene* scene, QWidget* parent = nullptr) : QGraphicsView(scene, parent),scene(scene) {
    setAcceptDrops(true);
    this->setRenderHint(QPainter::Antialiasing);

    this->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    base::GraphicalManager* manager = base::GraphicalManager::getInstance();

    scene->addItem(base::OptionBox::getInstance(nullptr));


}
void GraphicsView::dragEnterEvent(QDragEnterEvent* event){
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();  // 接受拖动操作
    }
    
}

void GraphicsView::dragMoveEvent(QDragMoveEvent* event){
    if (!(event->buttons() & Qt::LeftButton))
        return;

    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;

    // 设置文本数据
    mimeData->setText(event->mimeData()->text());

    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction | Qt::MoveAction);
    event->acceptProposedAction();  // 允许拖动操作继续
}

void GraphicsView::dropEvent(QDropEvent* event){
    if (event->mimeData()->hasText()) {
        QPointF dropPos = mapToScene(event->pos());  // 获取放下的位置
        base::GraphicalManager* manager = base::GraphicalManager::getInstance();
        QString text = event->mimeData()->text();
        if (text == "Elliesp") {
            manager->graphics.push_back(new base::Ellipse(dropPos.x(), dropPos.y(), 100, 100));
            scene->addItem(manager->graphics.back());
        }
        else if (text == "Rectangle") {
            manager->graphics.push_back(new base::Rectangle(dropPos.x(), dropPos.y(), 100, 100));
            scene->addItem(manager->graphics.back());
        }
        else if (text == "Allowline") {
            manager->graphics.push_back(new base::AllowLine(dropPos.x(), dropPos.y(),dropPos.x() + 100, dropPos.y() + 100));
            
            scene->addItem(manager->graphics.back());
        }
        else if (text == "Diamond") {
            manager->graphics.push_back(new base::Diamond(dropPos.x(), dropPos.y(), 100, 100));
            scene->addItem(manager->graphics.back());
        }
        else if (text == "RoundedRectangle") {
            manager->graphics.push_back(new base::RoundedRectangle(dropPos.x(), dropPos.y(), 100, 100,10,10));
            scene->addItem(manager->graphics.back());
        }
        else if (text == "Parallelogram") {
            manager->graphics.push_back(new base::Parallelogram(dropPos.x(), dropPos.y(), 100, 100, 80));
            scene->addItem(manager->graphics.back());
        }
        else if (text == "Start_or_Terminator") {
            manager->graphics.push_back(new base::Start_or_Terminator(dropPos.x(), dropPos.y(), 300, 50));
            scene->addItem(manager->graphics.back());
        }
        else if (text == "Subprocess") {
            manager->graphics.push_back(new base::Subprocess(dropPos.x(), dropPos.y(), 200, 100, 30));
            scene->addItem(manager->graphics.back());
        }
        else if (text == "Database") {
            manager->graphics.push_back(new base::Database(dropPos.x(), dropPos.y(), 200, 100, 0.3));
            scene->addItem(manager->graphics.back());
        }
        else if (text == "Document") {
            manager->graphics.push_back(new base::Document(dropPos.x(), dropPos.y(), 200, 100, 0.15));
            scene->addItem(manager->graphics.back());
        }
        else if (text == "DataStorage") {
            manager->graphics.push_back(new base::DataStorage(dropPos.x(), dropPos.y(), 200, 100, 15));
            scene->addItem(manager->graphics.back());
        }
        else if (text == "Textpointer") {
            manager->graphics.push_back(new base::Textpointer(scene, dropPos.x(), dropPos.y(), 200, 100));
        }
        base::OperationRecord* record = base::OperationRecord::getInstance();
        QVector<Object*> a;
        a.push_back(manager->graphics.back());
        record->recodeCommand(a, base::OperationRecord::Create);
        event->acceptProposedAction();  // 接受放下操作
    }

}
void GraphicsView::drawBackground(QPainter* painter, const QRectF& rect){
    QGraphicsView::drawBackground(painter, rect);

    if (m_drawGrid) {

        const int gridSize = 20; // 网格线间距
        const QColor gridColor = QColor(200, 200, 200); // 网格线颜色

        painter->setPen(gridColor);

        // 绘制垂直网格线
        for (int x = int(rect.left()) - (int(rect.left()) % gridSize); x < rect.right(); x += gridSize) {
            painter->drawLine(x, rect.top(), x, rect.bottom());
        }

        // 绘制水平网格线
        for (int y = int(rect.top()) - (int(rect.top()) % gridSize); y < rect.bottom(); y += gridSize) {
            painter->drawLine(rect.left(), y, rect.right(), y);
        }
    }
}

void GraphicsView::contextMenuEvent(QContextMenuEvent* event){
    QPointF scenePos = mapToScene(event->pos());
    base::OptionBox* box = base::OptionBox::getInstance();
    if (!box->parents.isEmpty()) {       
        // 转发事件给 QGraphicsItem
        QGraphicsSceneContextMenuEvent contextMenuEvent(QEvent::GraphicsSceneContextMenu);
        contextMenuEvent.setScenePos(scenePos);
        contextMenuEvent.setScreenPos(event->globalPos());
        contextMenuEvent.setModifiers(event->modifiers());
        box->parents[0]->contextMenuEvent(&contextMenuEvent);
        return;
    }
    QMenu contextMenu(this);

    // 创建一个 QCheckBox 并与 m_drawGrid 变量绑定
    QCheckBox* checkBox = new QCheckBox("显示网格线");
    checkBox->setChecked(m_drawGrid);

    // 当 QCheckBox 状态改变时更新 m_drawGrid
    connect(checkBox, &QCheckBox::toggled, this, [this](bool checked) {
        m_drawGrid = checked;
        this->update();  // 更新视图
        });

    // 使用 QWidgetAction 将 QCheckBox 添加到 QMenu
    QWidgetAction* checkBoxAction = new QWidgetAction(&contextMenu);
    checkBoxAction->setDefaultWidget(checkBox);

    contextMenu.addAction(checkBoxAction);
    contextMenu.exec(event->globalPos());
}
void GraphicsView::toPng(QString filename) {
    QGraphicsScene* scene = this->scene;
    if (!scene) return;

    // 获取视口大小
    QSize viewSize = this->viewport()->size();

    // 创建 QPixmap，大小为视口
    QPixmap pixmap(viewSize);

    // 确保背景填充正确
    pixmap.fill(Qt::white);  // 使用白色背景，或使用 Qt::transparent 如果想要透明背景

    // 渲染 QGraphicsView 的内容到 QPixmap
    QPainter painter(&pixmap);
    this->render(&painter);  // 渲染整个视图

    // 保存图片文件
    if (!pixmap.save(filename)) {
        qWarning("Failed to save PNG file: %s", qPrintable(filename));
    }
}
//导出svg
void GraphicsView::toSvg(QString filename) {
    if (!scene) return;

    // 获取场景的矩形区域
    QRectF sceneRect = scene->sceneRect();
    QSize viewSize = sceneRect.size().toSize();  // 使用场景矩形的尺寸

    // 设置 SVG 生成器
    QSvgGenerator generator;
    generator.setFileName(filename);
    generator.setSize(viewSize);
    generator.setViewBox(sceneRect);

    QPainter painter;
    if (!painter.begin(&generator)) {
        qWarning() << "Failed to begin painting with SVG generator.";
        return;
    }

    // 将 QGraphicsScene 的内容渲染到 SVG 文件
    scene->render(&painter, sceneRect, sceneRect);
    painter.end();

    qDebug() << "Scene has been exported to" << filename;
}
//打开svg：



void GraphicsView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() & Qt::LeftButton) {
        startPos = mapToScene(event->pos());
        QGraphicsItem* item = this->scene->itemAt(startPos, QTransform());

        if (!item) {
            base::OptionBox* box = base::OptionBox::getInstance();
            for (auto* it : box->parents) {
                if (it->textItem)
                    it->textItem->setTextInteractionFlags(Qt::NoTextInteraction);
            }

            box->clear();

            // 记录起始点
            lastPost = event->pos();
            // 创建选择框
            rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
            rubberBand->setGeometry(QRect(lastPost, QSize()));
            rubberBand->show();
        }

    }
    QGraphicsView::mousePressEvent(event);
}
void GraphicsView::mouseMoveEvent(QMouseEvent* event){
    if ((event->buttons() & Qt::LeftButton) && rubberBand ) {
        // 根据当前鼠标位置更新选择框大小
        rubberBand->setGeometry(QRect(lastPost, event->pos()).normalized());
        choose = true;
    }
    QGraphicsView::mouseMoveEvent(event);
}

// 鼠标释放事件，完成框选操作
void GraphicsView::mouseReleaseEvent(QMouseEvent* event){
    if (rubberBand) {
        // 将选择框的矩形区域转换到场景坐标系
        if (choose) {
            QRectF rubberBandRect = mapToScene(rubberBand->geometry()).boundingRect();


            base::GraphicalManager* manager = base::GraphicalManager::getInstance();
            for (auto* it : manager->graphics) {
                if (rubberBandRect.contains(it->mapRectToScene(it->showrectf))) {
                    base::OptionBox::getInstance(it);
                }
            }
        }
        

        // 删除选择框
        rubberBand->hide();
        delete rubberBand;
        rubberBand = nullptr;
        choose = false;
        update();
    }
    QGraphicsView::mouseReleaseEvent(event);
}
void GraphicsView::scaleSelectedItem(qreal factor)
{
    QList<QGraphicsItem*> selectedItems = scene->selectedItems();
    if (selectedItems.isEmpty()) {
        return; // 没有选中的元素
    }

    base::OptionBox* box = base::OptionBox::getInstance();
    for (auto* item : box->parents) {
        QRectF boundingRect = item->boundingRect(); // 获取项的边界矩形
        QPointF itemCenter = boundingRect.center(); // 计算图形的中心点

        // 获取当前变换
        QTransform oldTransform = item->transform();

        // 创建新的变换
        QTransform newTransform;
        newTransform.translate(itemCenter.x(), itemCenter.y()); // 平移到中心点
        newTransform.scale(factor, factor); // 应用缩放
        newTransform.translate(-itemCenter.x(), -itemCenter.y()); // 平移回原位置

        // 设置新的变换
        item->setTransform(newTransform * oldTransform);
    }

}

//旋转
void GraphicsView::rotateSelectedItem(qreal angleDelta) {
    base::OptionBox* box = base::OptionBox::getInstance();
    base::OperationRecord* record = base::OperationRecord::getInstance();
    record->recodeCommand(box->parents, base::OperationRecord::Trans);

    for (auto* item : box->parents) {
        QRectF boundingRect = item->boundingRect(); // 获取项的边界矩形
        QPointF itemCenter = boundingRect.center(); // 计算图形的中心点

        // 保存当前变换
        QTransform oldTransform = item->transform();

        // 计算旋转变换
        QTransform rotationTransform;
        rotationTransform.translate(itemCenter.x(), itemCenter.y()); // 平移到中心点
        rotationTransform.rotate(angleDelta); // 旋转
        rotationTransform.translate(-itemCenter.x(), -itemCenter.y()); // 平移回原位置

        // 应用旋转变换
        item->setTransform(oldTransform * rotationTransform);
    }
    
}

//旋转缩放

void GraphicsView::zoomOut()
{
    scale(1.2, 1.2);

}

void GraphicsView::zoomIn()
{
    scale(1.0 / 1.2, 1.0 / 1.2);

}

void GraphicsView::rotateLeft()
{
    rotationAngle -= 90; // 旋转角度可以根据需要调整
    setRotation(rotationAngle);
}

void GraphicsView::rotateright()
{
    rotationAngle += 90; // 旋转角度可以根据需要调整
    setRotation(rotationAngle);
}

void GraphicsView::setRotation(qreal angle)
{
    // 旋转 QGraphicsView
    QTransform transform;
    transform.rotate(angle);
    setTransform(transform);

}
}



