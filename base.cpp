#include "base.h"
#include "graphcopy.h"
#include "mainwindow.h"
#include "OperationRecord.h"
#include <Windows.h>
namespace base {
    OptionBox* OptionBox::unique = nullptr;
    QMenu* Object::menu = nullptr;
    void Object::setRect(QRectF rect) {
        this->rectf = rect;
        this->showrectf = QRectF(rectf.x() + rectfwidth, rectf.y() + rectfwidth, rectf.width() - 2 * rectfwidth, rectf.height() + 2 * rectfwidth);
    }
    void Object::setShowRect(QRectF rect) {
        this->showrectf = rect;
        this->rectf = QRectF(showrectf.x() - rectfwidth, showrectf.y() - rectfwidth, showrectf.width() + 2 * rectfwidth, showrectf.height() + 2 * rectfwidth);
    }
    void Object::setBrush(QBrush brush) {
        this->brush = brush;
        update();
    }
    void Object::setWidth(int width) {
        this->penWidth = width;
        update();
    }
    void Object::setColor(QColor color) {
        this->color = color;
        update();
    }
    Object::Object()
    {
        if (!menu) {
            menu = new QMenu();
            QAction* copyAction = menu->addAction("复制");
            QAction* pasteAction = menu->addAction("粘贴");
            QAction* cutAction = menu->addAction("剪切");
            QAction* deleteAction = menu->addAction("删除");
            QAction* upAction = menu->addAction("前置");
            QAction* downAction = menu->addAction("后置");
            //向上移
            connect(upAction, &QAction::triggered, this, [this]() {
                base::OptionBox* box = base::OptionBox::getInstance();
                for (auto* it : box->parents) {
                    it->setZValue(it->zValue() + 1);
                }
                });
            //向下移
            connect(downAction, &QAction::triggered, this, [this]() {
                base::OptionBox* box = base::OptionBox::getInstance();
                for (auto* it : box->parents) {
                    it->setZValue(it->zValue() - 1);
                }
                });
            //复制
            connect(copyAction, &QAction::triggered, this, [this]() {
                base::OptionBox* box = base::OptionBox::getInstance();
                base::CopyManager* copym = base::CopyManager::getInstance();
                copym->clear();
                for (auto* it : box->parents) {
                    copym->add(it);
                }
                });
            //粘贴
            connect(pasteAction, &QAction::triggered, this, [this]() {
                base::CopyManager* copym = base::CopyManager::getInstance();
                if (!copym->childs.isEmpty()) {
                    base::GraphicalManager* manager = base::GraphicalManager::getInstance();
                    for (auto* it : copym->childs) {

                        manager->graphics.push_back(new base::Object(it));
                        manager->view->scene()->addItem(manager->graphics.back());
                    }
                }
                });
            //剪切
            connect(cutAction, &QAction::triggered, this, [this]() {
                base::OptionBox* box = base::OptionBox::getInstance();
                base::CopyManager* copym = base::CopyManager::getInstance();
                copym->clear();
                for (auto* it : box->parents) {
                    copym->add(it);
                }
                base::GraphicalManager* manager = base::GraphicalManager::getInstance();
                for (auto* it : box->parents) {
                    auto t = std::find(manager->graphics.begin(), manager->graphics.end(), it);

                    if (t != manager->graphics.end()) {
                        manager->graphics.erase(t);
                        delete it;
                        it = nullptr;
                    }
                }
                box->clear();
                });
            //删除
            connect(deleteAction, &QAction::triggered, this, [this] {
                base::OptionBox* box = base::OptionBox::getInstance();
                base::CopyManager* copym = base::CopyManager::getInstance();
                copym->clear();
                base::GraphicalManager* manager = base::GraphicalManager::getInstance();
                for (auto* it : box->parents) {
                    auto t = std::find(manager->graphics.begin(), manager->graphics.end(), it);

                    if (t != manager->graphics.end()) {
                        manager->graphics.erase(t);
                        delete it;
                        it = nullptr;
                    }
                }
                box->clear();
                });
        }     
        setFlag(QGraphicsItem::ItemIsMovable);
        setFlag(ItemIsSelectable);
        setColor(QColor(0, 0, 0, 255));
        setWidth(2);
        setBrush(QBrush(QColor(255, 255, 255, 0)));
        this->rectfwidth = 10;
    }

    bool Object::operator<(const Object* t)
    {
        return this->zValue() < t->zValue();
    }

    Object::Object(const Object* t)
    {
        this->rectfwidth = 10;
        setFlag(ItemIsMovable);
        setFlag(ItemIsSelectable);
        setWidth(2);
        this->path = t->path;
        this->setRect(t->rectf);
        this->setColor(t->color);
        this->setBrush(t->brush);
        this->setTransform(t->transform());
        this->setPos(t->pos());
        for (auto it : t->nodes) {
            this->nodes.push_back(new Node(*it,this));
        }
    }

    void Object::setFontBold(bool bold)
    {
        QFont font = textItem->font();
        font.setBold(bold);
        textItem->setFont(font);
        scene()->addItem(textItem);
    }

    void Object::mousePressEvent(QGraphicsSceneMouseEvent* event) {
        if (event->buttons() == Qt::LeftButton) {
            mouselastscenePos = event->scenePos();
            mouselastPos = event->pos();
            this->resize = getPosition(event);
            base::OptionBox::getInstance(this);
            update();
        }
        QGraphicsItem::mousePressEvent(event); // 调用基类的处理函数
    }

    void Object::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
        if (event->buttons() & Qt::LeftButton) {
            QPointF nowscenePos = event->scenePos();
            QPointF nowPos = event->pos();
            if (!ifrecord) {
                ifrecord = true;
                base::OperationRecord* recode = base::OperationRecord::getInstance();
                base::OptionBox* box = base::OptionBox::getInstance();
                recode->recodeCommand(box->parents, base::OperationRecord::Trans);
            }
            //right
            if (this->resize == 0) {
                this->showrectf.setRight(nowPos.x());
                graphics_Update(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height());
            }
            //left
            else if (this->resize == 1) {
                this->showrectf.setLeft(nowPos.x());
                graphics_Update(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height());
            }
            //top
            else if (this->resize == 2) {
                this->showrectf.setTop(nowPos.y());
                graphics_Update(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height());
            }
            //button
            else if (this->resize == 3) {
                this->showrectf.setBottom(nowPos.y());
                graphics_Update(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height());
            }
            else {
                
                QPointF dpos = nowscenePos - mouselastscenePos;
                base::OptionBox* box = base::OptionBox::getInstance();
                for (auto* t : box->parents) {
                    qDebug() << box->parents.size();
                    //t->setTransform(t->transform().translate(dpos.x(), dpos.y()));
                    t->setPos(t->pos() + dpos);
                }

                update();
            }
            mouselastscenePos = nowscenePos;
            mouselastPos = nowPos;
            for (auto* it : nodes) {
                it->AllowLine_Update();
            }
            update();
        }


    }

    void Object::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
        ifrecord = false;
        // 处理鼠标释放事件
        QGraphicsItem::mouseReleaseEvent(event); // 调用基类的处理函数

        this->resize = -1;
        update();
    }
    /***********************************************************************///更改部分
    void Object::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
    {
        if (!textItem) {
            textItem = new QGraphicsTextItem(this);
            textItem->setTextInteractionFlags(Qt::TextEditorInteraction);
            textItem->setPlainText("请输入文本"); // 初始文本

            // 将 textItem 添加到场景，而不是作为子项（除非有特定需求）
            scene()->addItem(textItem);

            // 连接到文本改变信号，以更新位置
            connect(textItem->document(), &QTextDocument::contentsChanged, this, &Object::updateTextItemPosition);

            // 初始位置设置
            updateTextItemPosition();
        }
        else {
            textItem->setTextInteractionFlags(Qt::TextEditorInteraction);
        }
        // 标记事件已处理
        event->accept();
    }

    void Object::updateTextItemPosition() {

        // 获取文本项的边界框
        QRectF textRect = textItem->boundingRect();

        // 计算文本项应放置的位置，以保持其在 rectf 的中心
        QPointF center = showrectf.center();
        QPointF newPos(center.x() - textRect.width() / 2, center.y() - textRect.height() / 2);

        newPos.setY(showrectf.top() + (rectf.height() - textRect.height()) / 2);
        
        // 设置文本项的新位置
        textItem->setPos(newPos);
    }

    /******************************************************************************/
    QRectF Object::boundingRect() const {
        return QRectF(rectf);
    }
    void Object::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
        QPen pen(color);
        pen.setWidth(penWidth);
        pen.setCosmetic(true);
        
        painter->setBrush(brush);
        painter->setPen(pen);
        painter->drawPath(path);
    }
    void Object::graphics_Update(int x1, int y1, int x2, int y2)
    {
    }

    void Object::pos_Update(int x1, int y1, int x2, int y2)
    {
    }

    void Object::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {

        if (menu) {
            QAction* selectedAction = menu->exec(event->screenPos());  // 在鼠标位置显示菜单
        }
    }
    Object* Object::clone()
    {
        return new Object(this);
    }
    QString Object::graphicsName()
    {
        return "Object";
    }

    void Object::save(QDataStream& out)
    {
        try {
            out << graphicsName();
            out << this->showrectf;
            out << this->color;
            out << this->penWidth;
            out << this->brush;
            out << this->pos();
            out << this->transform();

        }
        catch(const std::exception& e){
            qDebug() << e.what();
        }
        
        
    }
    Object::~Object() {
        
        for (auto* it : nodes) {
            for (auto* t : it->vecHead) {
                for (auto* q : t->nodes) {
                    q->connects.remove(it);
                }
            }
            for (auto* t : it->vecTail) {
                for (auto* q : t->nodes) {
                    q->connects.remove(it);
                }
            }
        }
        
    }
    int Object::getPosition(QGraphicsSceneMouseEvent* event)
    {
        qreal left = rectf.left();   // 左边界
        qreal right = rectf.right(); // 右边界
        qreal top = rectf.top();     // 上边界
        qreal bottom = rectf.bottom(); // 下边界
        QVector<QRectF> areas;
        int w = 20;
        areas.push_back(QRectF(right - w, top + w, w, bottom - top - w * 2));//right
        areas.push_back(QRectF(left, top + w, w, bottom - top - w * 2));//left
        areas.push_back(QRectF(left + w, top, right - left - w * 2, w));//top
        areas.push_back(QRectF(left + w, bottom - w, right - left - w * 2, w));//buttom

        areas.push_back(QRectF(left, top, w, w));//left top 4
        areas.push_back(QRectF(left, bottom - w, w, w));//left bottom
        areas.push_back(QRectF(right - w, top, w, w));//right top
        areas.push_back(QRectF(right - w, bottom - w, w, w));//right bottom 7

        for (auto it : nodes) {
            areas.push_back(it->rect);
        }
        for (int i = areas.size() - 1; i > -1; i--) {
            if (areas[i].contains(event->pos())) return i;
        }
        return -1;
    }
    OptionBox::OptionBox(Object* parent) :Object() {
        this->setZValue(0);
        this->setColor(QColor(0, 0, 0, 255));
        this->setBrush(QBrush(QColor(255, 255, 255, 0)));
        this->setWidth(3);
        this->addObject(parent);
    }
    OptionBox* OptionBox::getInstance(Object* parent)
    {
        if (!unique) {
            unique = new OptionBox(parent);
        }
        else unique->addObject(parent);
        return unique;
    }
    OptionBox* OptionBox::getInstance()
    {
        return unique;
    }
    void OptionBox::addObject(Object* parent) {
        if (!parent) return;
        auto t = std::find(parents.begin(), parents.end(), parent);
        if (t != parents.end()) return;

        this->parents.push_back(parent);
        if(parents.size()==1)
            this->setRect(parent->rectf);
        else {
            this->setRect(this->rectf.united(parent->rectf));
        }
        this->path.clear();
        this->path.addRect(this->rectf);
        update();
    }
    void OptionBox::clear()
    {
        QVector<Object*>().swap(unique->parents);
    }

    void OptionBox::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
        if (parents.isEmpty()) return;
        QPen pen(color);
        pen.setWidth(penWidth);
        pen.setCosmetic(true);
        pen.setCosmetic(true);
        pen.setStyle(Qt::DotLine);
        painter->setBrush(brush);
        painter->setPen(pen);
        painter->setBrush(QColor(0, 0, 0, 0));
        this->path.clear();
        this->setRect(parents[0]->mapRectToParent(parents[0]->rectf));
        for (int i = 1; i < parents.size();i++) {
            this->setRect(this->rectf.united(parents[i]->mapRectToParent(parents[i]->rectf)));
        }
        for (auto* it : parents) {
            this->path.addRect(it->mapRectToParent(it->rectf));
            for (auto* t : it->nodes) {
                this->path.addRect(it->mapRectToParent(t->rect));
            }
            
        }
        painter->drawPath(path);

    }
    Ellipse::Ellipse(int x, int y, int width, int height) :Object() {
        this->setZValue(1);
        nodes.push_back(new Node(QPointF(x, y + height / 2),this));
        nodes.push_back(new Node(QPointF(x + width / 2, y),this));
        nodes.push_back(new Node(QPointF(x + width / 2, y + height),this));
        nodes.push_back(new Node(QPointF(x + width, y + height / 2),this));

        this->setShowRect(QRectF(x, y, width, height));
        this->path.addEllipse(this->showrectf);
    }
    Ellipse::Ellipse(const Ellipse* t):Object(t)
    {
    }
    void Ellipse::graphics_Update(int x, int y, int width, int height)
    {
        nodes[0]->setPos(QPointF(x, y + height / 2));
        nodes[1]->setPos(QPointF(x + width / 2, y));
        nodes[2]->setPos(QPointF(x + width / 2, y + height));
        nodes[3]->setPos(QPointF(x + width, y + height / 2));
        this->setShowRect(QRectF(x, y, width, height));
        this->path.clear();
        this->path.addEllipse(this->showrectf);
    }
    Object* Ellipse::clone()
    {
        return new Ellipse(this);
    }
    QString Ellipse::graphicsName()
    {
        return "Ellipse";
    }
    Rectangle::Rectangle(int x, int y, int width, int height) :Object() {
        nodes.push_back(new Node(QPointF(x, y + height / 2), this));
        nodes.push_back(new Node(QPointF(x + width / 2, y), this));
        nodes.push_back(new Node(QPointF(x + width / 2, y + height), this));
        nodes.push_back(new Node(QPointF(x + width, y + height / 2), this));
        this->setZValue(1);
        this->setShowRect(QRectF(x, y, width, height));
        this->path.addRect(this->showrectf);
    }
    Rectangle::Rectangle(const Rectangle* t):Object(t)
    {
    }
    void Rectangle::graphics_Update(int x, int y, int width, int height)
    {
        this->setShowRect(QRectF(x, y, width, height));
        nodes[0]->setPos(QPointF(x, y + height / 2));
        nodes[1]->setPos(QPointF(x + width / 2, y));
        nodes[2]->setPos(QPointF(x + width / 2, y + height));
        nodes[3]->setPos(QPointF(x + width, y + height / 2));
        this->path.clear();
        this->path.addRect(this->showrectf);
    }
    Object* Rectangle::clone()
    {
        return new Rectangle(this);
    }
    QString Rectangle::graphicsName()
    {
        return "Rectangle";
    }
    Diamond::Diamond(int x, int y, int width, int height) :Object() {
        this->setZValue(1);
        nodes.push_back(new Node(QPointF(x, y + height / 2), this));
        nodes.push_back(new Node(QPointF(x + width / 2, y), this));
        nodes.push_back(new Node(QPointF(x + width / 2, y + height), this));
        nodes.push_back(new Node(QPointF(x + width, y + height / 2), this));
        this->setShowRect(QRectF(x, y, width, height));
        this->path.moveTo(x + width / 2, y);
        this->path.lineTo(x, y + height / 2);
        this->path.lineTo(x + width / 2, y + height);
        this->path.lineTo(x + width, y + height / 2);
        this->path.lineTo(x + width / 2, y);
        this->path.closeSubpath();
    }
    Diamond::Diamond(const Diamond* t):Object(t)
    {
    }
    void Diamond::graphics_Update(int x, int y, int width, int height)
    {
        nodes[0]->setPos(QPointF(x, y + height / 2));
        nodes[1]->setPos(QPointF(x + width / 2, y));
        nodes[2]->setPos(QPointF(x + width / 2, y + height));
        nodes[3]->setPos(QPointF(x + width, y + height / 2));
        this->setShowRect(QRectF(x, y, width, height));
        this->path.clear();
        this->path.moveTo(x + width / 2, y);
        this->path.lineTo(x, y + height / 2);
        this->path.lineTo(x + width / 2, y + height);
        this->path.lineTo(x + width, y + height / 2);
        this->path.lineTo(x + width / 2, y);
        this->path.closeSubpath();
    }
    Object* Diamond::clone()
    {
        return new Diamond(this);
    }

    QString Diamond::graphicsName()
    {
        return "Diamond";
    }



    //箭头
    void AllowLine::createArrowHead(QPointF start, QPointF end, QPainterPath& path) {
        double angle = qAtan2(end.y() - start.y(), end.x() - start.x());
        qreal arrowSize = this->penWidth * 3;

        QPointF arrowP1 = end - QPointF(arrowSize * qCos(angle + M_PI / 6), arrowSize * qSin(angle + M_PI / 6));
        QPointF arrowP2 = end - QPointF(arrowSize * qCos(angle - M_PI / 6), arrowSize * qSin(angle - M_PI / 6));
        path.moveTo(end);
        path.lineTo(arrowP1);
        path.lineTo(arrowP2);
        path.lineTo(end);
        path.closeSubpath();
    }
    AllowLine::AllowLine(int x1, int y1, int x2, int y2) :Object()
    {
        this->nodes.push_back(new Node(QPointF(x1, y1),this));
        this->nodes.push_back(new Node(QPointF(x2, y2),this));
        QPointF start = nodes[0]->point;
        QPointF end = nodes[1]->point;
        this->setZValue(1);

        this->setBrush(QColor(0, 0, 0, 255));

        this->setShowRect(QRectF(start, end));
        this->path.moveTo(start);
        this->path.lineTo(end);
        path.closeSubpath();

        createArrowHead(start, end, path);

    }

    AllowLine::AllowLine(const AllowLine* t):Object(t)
    {
    }

    AllowLine::~AllowLine()
    {
		if (!nodes[0]->connects.isEmpty()) {
            for (auto* it : nodes[0]->connects) {
                it->vecHead.remove(this);
                it->connects.remove(nodes[0]);
            }


		}
        if (!nodes[1]->connects.isEmpty()) {
            for (auto* it : nodes[1]->connects) {
                it->vecTail.remove(this);
                it->connects.remove(nodes[0]);
            }
        }

    }

    void AllowLine::mousePressEvent(QGraphicsSceneMouseEvent* event) {
        Object::mousePressEvent(event);      
    }

    void AllowLine::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
        if (event->buttons() & Qt::LeftButton) {
            QPointF nowscenePos = event->scenePos();
            QPointF nowPos = event->pos();
            if (!ifrecord) {
                ifrecord = true;
                base::OperationRecord* recode = base::OperationRecord::getInstance();
                base::OptionBox* box = base::OptionBox::getInstance();
                recode->recodeCommand(box->parents, base::OperationRecord::Trans);
            }
            //right
            if (this->resize == 0) {
                this->showrectf.setRight(nowPos.x());
                graphics_Update(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height());
            }
            //left
            else if (this->resize == 1) {
                this->showrectf.setLeft(nowPos.x());
                graphics_Update(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height());
            }
            //top
            else if (this->resize == 2) {
                this->showrectf.setTop(nowPos.y());
                graphics_Update(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height());
            }
            //button
            else if (this->resize == 3) {
                this->showrectf.setBottom(nowPos.y());
                graphics_Update(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height());
            }
            //node0
            else if (this->resize == 8) {
                nodes[0]->setPos(nowPos);
                pos_Update(nodes[0]->point.x(), nodes[0]->point.y(), nodes[1]->point.x(), nodes[1]->point.y());
                for (auto*& graph : base::GraphicalManager::getInstance()->graphics) {
                    if (graph == this) continue;
                    for (auto* it : graph->nodes) {
                        if (graph->mapRectToParent(it->rect).contains(this->mapToParent(event->pos()))) {
                            nodes[0]->setPos(graph->mapRectToItem(this, it->rect).center());
                            pos_Update(nodes[0]->point.x(), nodes[0]->point.y(), nodes[1]->point.x(), nodes[1]->point.y());
                            break;
                        }
                    }
                }
            }
            //node1
            else if (this->resize == 9) {
                nodes[1]->setPos(nowPos);
                pos_Update(nodes[0]->point.x(), nodes[0]->point.y(), nodes[1]->point.x(), nodes[1]->point.y());
                for (auto*& graph : base::GraphicalManager::getInstance()->graphics) {
                    if (graph == this) continue;
                    for (auto* it : graph->nodes) {
                        if (graph->mapRectToParent(it->rect).contains(this->mapToParent(event->pos()))) {
                            nodes[1]->setPos(graph->mapRectToItem(this, it->rect).center());
                            pos_Update(nodes[0]->point.x(), nodes[0]->point.y(), nodes[1]->point.x(), nodes[1]->point.y());
                            break;
                        }
                    }
                }
            }
            else {

                QGraphicsItem::mouseMoveEvent(event); // 调用基类的处理函数
                QPointF dpos = nowscenePos - mouselastscenePos;
                base::OptionBox* box = base::OptionBox::getInstance();
                for (auto* t : box->parents) {
                    qDebug() << box->parents.size();
                    if (t != this)
                        t->setTransform(t->transform().translate(dpos.x(), dpos.y()));
                    //t->setPos(t->pos() + dpos);
                }
                update();

            }
            qDebug() << resize;
            mouselastscenePos = nowscenePos;
            mouselastPos = nowPos;
            update();
        }
    }

    void AllowLine::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
		if (resize == 8) {
			nodes[0]->removeConnect();
			for (auto*& graph : base::GraphicalManager::getInstance()->graphics) {
				if (graph == this) continue;
				for (auto*& it : graph->nodes) {
					if (graph->mapRectToParent(it->rect).contains(this->mapToParent(event->pos()))) {
						nodes[0]->addConnect(it);
						it->vecHead.insert(this);
					}
				}
			}
		}
		else if (resize == 9) {
			nodes[1]->removeConnect();
			for (auto*& graph : base::GraphicalManager::getInstance()->graphics) {
				if (graph == this) continue;
				for (auto*& it : graph->nodes) {
					if (graph->mapRectToParent(it->rect).contains(this->mapToParent(event->pos()))) {
						nodes[1]->addConnect(it);
						it->vecTail.insert(this);
					}
				}
			}
		}
		else {
			nodes[0]->removeConnect();
			nodes[1]->removeConnect();
		}
		Object::mouseReleaseEvent(event);

	}

    Object* AllowLine::clone()
    {
        return new AllowLine(this);
    }
    QString AllowLine::graphicsName()
    {
        return "AllowLine";
    }
    void AllowLine::save(QDataStream& out)
    {
        try {
            out << graphicsName();
            out << this->showrectf;
            out << this->nodes[0]->point;
            out << this->nodes[1]->point;
           
            out << this->color;
            out << this->penWidth;
            out << this->brush;
            out << this->pos();
            out << this->transform();

        }
        catch (const std::exception& e) {
            qDebug() << e.what();
        }
    }
    void AllowLine::graphics_Update(int x, int y, int width, int height)
    {
        nodes[0]->setPos(QPointF(x, y));
        nodes[1]->setPos(QPointF(x + width, y + height));
        this->path.clear();
        this->setShowRect(QRectF(nodes[0]->point, nodes[1]->point));
        this->path.moveTo(nodes[0]->point);
        this->path.lineTo(nodes[1]->point);
        path.closeSubpath();
        createArrowHead(nodes[0]->point, nodes[1]->point, path);
    }

    void AllowLine::pos_Update(int x1, int y1, int x2, int y2)
    {
        nodes[0]->setPos(QPointF(x1, y1));
        nodes[1]->setPos(QPointF(x2, y2));
        this->path.clear();
        if (x1 > x2 && y1 < y2) {
            this->setShowRect(QRectF(QPointF(x2, y1), QPointF(x1, y2)));
        }
        else if (x1 > x2, y1 > y2) {
            this->setShowRect(QRectF(QPointF(x2, y2), QPointF(x1, y1)));
        }
        else if (x1 < x2, y1 > y2) {
            this->setShowRect(QRectF(QPointF(x1, y2), QPointF(x2, y1)));
        }
        else
            this->setShowRect(QRectF(QPointF(x1, y1), QPointF(x2, y2)));

        this->path.moveTo(nodes[0]->point);
        this->path.lineTo(nodes[1]->point);
        path.closeSubpath();
        createArrowHead(nodes[0]->point, nodes[1]->point, path);
    }

    Textpointer::Textpointer(QGraphicsScene* scene, int x, int y, int width, int height) :Object() {
        this->setShowRect(QRectF(x, y, width, height));
        this->setColor(QColor(0, 0, 0, 0));
        this->path.addRect(this->showrectf);
        this->setZValue(1);
        // 创建一个文本项
        textItem = new QGraphicsTextItem(this);
        textItem->setTextInteractionFlags(Qt::TextEditorInteraction);
        textItem->setPlainText("请输入文本"); // 初始文本
        // 将文本项添加到场景中，并设置其位置
        textItem->setPos(x, y);
        textItem->setZValue(0);
        scene->addItem(this);
        scene->addItem(textItem);
    }
    Textpointer::Textpointer(const Textpointer* t):Object(t)
    {
    }
    void Textpointer::graphics_Update(int x, int y, int width, int height)
    {
        this->setShowRect(QRectF(x, y, width, height));
        this->path.clear();
        // 更新文本项的位置
        textItem->setPos(x, y);
        this->path.addRect(this->showrectf);
    }

    Object* Textpointer::clone()
    {
        return new Textpointer(this);
    }

    QString Textpointer::graphicsName()
    {
        return "Textpointer";
    }

    int RoundedRectangle::rx = 0;
    int RoundedRectangle::ry = 0;
    RoundedRectangle::RoundedRectangle(int x, int y, int width, int height, int rx, int ry)
    {
        nodes.push_back(new Node(QPointF(x, y + height / 2), this));
        nodes.push_back(new Node(QPointF(x + width / 2, y), this));
        nodes.push_back(new Node(QPointF(x + width / 2, y + height), this));
        nodes.push_back(new Node(QPointF(x + width, y + height / 2), this));
        RoundedRectangle::rx = rx;
        RoundedRectangle::ry = ry;
        this->setZValue(1);

        this->setShowRect(QRectF(x, y, width, height));
        this->path.addRoundedRect(this->showrectf, rx, ry);

    }
    RoundedRectangle::RoundedRectangle(const RoundedRectangle* t):Object(t)
    {
    }
    void RoundedRectangle::graphics_Update(int x, int y, int width, int height)
    {
        nodes[0]->setPos(QPointF(x, y + height / 2));
        nodes[1]->setPos(QPointF(x + width / 2, y));
        nodes[2]->setPos(QPointF(x + width / 2, y + height));
        nodes[3]->setPos(QPointF(x + width, y + height / 2));
        this->setShowRect(QRectF(x, y, width, height));
        this->path.clear();
        this->path.addRoundedRect(this->showrectf, rx, ry);
    }
    Object* RoundedRectangle::clone()
    {
        return new RoundedRectangle(this);
    }
    QString RoundedRectangle::graphicsName()
    {
        return "RoundedRectangle";
    }
    float Parallelogram::t = 0;
    Parallelogram::Parallelogram(int x, int y, int width, int height, float t)
    {
        Parallelogram::t = t;
        this->setZValue(1);

        int dwidth = height / qTan(qDegreesToRadians(t));
        QPointF p1(x + dwidth, y);
        QPointF p2(x + width, y);
        QPointF p3(x + width - dwidth, y + height);
        QPointF p4(x, y + height);
        nodes.push_back(new Node((p1 + p2) / 2, this));
        nodes.push_back(new Node((p2 + p3) / 2, this));
        nodes.push_back(new Node((p3 + p4) / 2, this));
        nodes.push_back(new Node((p1 + p4) / 2, this));
        this->setShowRect(QRectF(x, y, width, height));
        this->path.moveTo(p1);
        this->path.lineTo(p2);
        this->path.lineTo(p3);
        this->path.lineTo(p4);
        this->path.lineTo(p1);
        this->path.closeSubpath();
    }
    Parallelogram::Parallelogram(const Parallelogram* t):Object(t)
    {
    }
    void Parallelogram::graphics_Update(int x, int y, int width, int height)
    {
        int dwidth = height / qTan(qDegreesToRadians(t));
        QPointF p1(x + dwidth, y);
        QPointF p2(x + width, y);
        QPointF p3(x + width - dwidth, y + height);
        QPointF p4(x, y + height);
        nodes[0]->setPos((p1 + p2) / 2);
        nodes[1]->setPos((p2 + p3) / 2);
        nodes[2]->setPos((p3 + p4) / 2);
        nodes[3]->setPos((p1 + p4) / 2);
        this->setShowRect(QRectF(x, y, width, height));
        this->path.clear();
        this->path.moveTo(p1);
        this->path.lineTo(p2);
        this->path.lineTo(p3);
        this->path.lineTo(p4);
        this->path.lineTo(p1);
        this->path.closeSubpath();
    }
    Object* Parallelogram::clone()
    {
        return new Parallelogram(this);
    }
    QString Parallelogram::graphicsName()
    {
        return "Parallelogram";
    }
    Start_or_Terminator::Start_or_Terminator(int x, int y, int width, int height)

    {
        nodes.push_back(new Node(QPointF(x + width / 2, y), this));
        nodes.push_back(new Node(QPointF(x + width / 2, y + height), this));
        this->setZValue(1);

        this->setShowRect(QRectF(x, y, width, height));

        this->path.arcMoveTo(QRectF(x, y, height, height),90);
        this->path.arcTo(QRectF(x, y, height, height), 90, 180);

        this->path.moveTo(x + height / 2, y + height);
        this->path.lineTo(x + width - height / 2, y + height);

        this->path.arcMoveTo(QRectF(x + width - height, y, height, height), -90);
        this->path.arcTo(QRectF(x + width - height, y, height, height), -90, 180);

        this->path.moveTo(x + width - height / 2, y);
        this->path.lineTo(x + height / 2, y);
        this->path.closeSubpath();
    }
    Start_or_Terminator::Start_or_Terminator(const Start_or_Terminator* t) :Object(t)
    {
    }
    void Start_or_Terminator::graphics_Update(int x, int y, int width, int height)
    {
        nodes[0]->setPos(QPointF(x + width / 2, y));
        nodes[1]->setPos(QPointF(x + width / 2, y + height));
        this->setShowRect(QRectF(x, y, width, height));
        this->path.clear();
        this->path.arcMoveTo(QRectF(x, y, height, height), 90);
        this->path.arcTo(QRectF(x, y, height, height), 90, 180);

        this->path.moveTo(x + height / 2, y + height);
        this->path.lineTo(x + width - height / 2, y + height);

        this->path.arcMoveTo(QRectF(x + width - height, y, height, height), -90);
        this->path.arcTo(QRectF(x + width - height, y, height, height), -90, 180);

        this->path.moveTo(x + width - height / 2, y);
        this->path.lineTo(x + height / 2, y);
        this->path.closeSubpath();
    }
    Object* Start_or_Terminator::clone()
    {
        return new Start_or_Terminator(this);
    }
    QString Start_or_Terminator::graphicsName()
    {
        return "Start_or_Terminator";
    }
    int Subprocess::dwidth = 0;
    Subprocess::Subprocess(int x, int y, int width, int height, int dwidth)
    {
        nodes.push_back(new Node(QPointF(x, y + height / 2), this));
        nodes.push_back(new Node(QPointF(x + width / 2, y), this));
        nodes.push_back(new Node(QPointF(x + width / 2, y + height), this));
        nodes.push_back(new Node(QPointF(x + width, y + height / 2), this));
        Subprocess::dwidth = dwidth;
        this->setZValue(1);

        this->setShowRect(QRectF(x, y, width, height));
        this->path.addRect(this->showrectf);
        this->path.moveTo(x + dwidth, y);
        this->path.lineTo(x + dwidth, y + height);
        this->path.moveTo(x + width - dwidth, y);
        this->path.lineTo(x + width - dwidth, y + height);
        this->path.closeSubpath();
    }
    Subprocess::Subprocess(const Subprocess* t):Object(t)
    {
    }
    void Subprocess::graphics_Update(int x, int y, int width, int height)
    {
        nodes[0]->setPos(QPointF(x, y + height / 2));
        nodes[1]->setPos(QPointF(x + width / 2, y));
        nodes[2]->setPos(QPointF(x + width / 2, y + height));
        nodes[3]->setPos(QPointF(x + width, y + height / 2));
        this->setShowRect(QRectF(x, y, width, height));
        this->path.clear();
        this->path.addRect(this->showrectf);
        this->path.moveTo(x + dwidth, y);
        this->path.lineTo(x + dwidth, y + height);
        this->path.moveTo(x + width - dwidth, y);
        this->path.lineTo(x + width - dwidth, y + height);
        this->path.closeSubpath();
    }
    Object* Subprocess::clone()
    {
        return new Subprocess(this);
    }
    QString Subprocess::graphicsName()
    {
        return "Subprocess";
    }
    float Database::t = 0;
    Database::Database(int x, int y, int width, int height, float t)
    {
        nodes.push_back(new Node(QPointF(x, y + height / 2), this));
        nodes.push_back(new Node(QPointF(x + width / 2, y), this));
        nodes.push_back(new Node(QPointF(x + width / 2, y + height), this));
        nodes.push_back(new Node(QPointF(x + width, y + height / 2), this));
        Database::t = t;
        this->setZValue(1);

        this->setShowRect(QRectF(x, y, width, height));
        int dheight = height * t;
        this->path.addEllipse(x, y, width, dheight);
        this->path.arcMoveTo(QRectF(x, y + height - dheight, width, dheight), 180);
        this->path.arcTo(x, y + height - dheight, width, dheight, 180, 180);
        this->path.moveTo(x, y + dheight / 2);
        this->path.lineTo(x, y + height - dheight / 2);
        this->path.moveTo(x + width, y + dheight / 2);
        this->path.lineTo(x + width, y + height - dheight / 2);
        this->path.closeSubpath();
    }
    Database::Database(const Database* t):Object(t)
    {
    }
    void Database::graphics_Update(int x, int y, int width, int height)
    {
        nodes[0]->setPos(QPointF(x, y + height / 2));
        nodes[1]->setPos(QPointF(x + width / 2, y));
        nodes[2]->setPos(QPointF(x + width / 2, y + height));
        nodes[3]->setPos(QPointF(x + width, y + height / 2));
        this->setShowRect(QRectF(x, y, width, height));
        this->path.clear();
        int dheight = height * t;
        this->path.addEllipse(x, y, width, dheight);
        this->path.arcMoveTo(QRectF(x, y + height - dheight, width, dheight), 180);
        this->path.arcTo(x, y + height - dheight, width, dheight, 180, 180);
        this->path.moveTo(x, y + dheight / 2);
        this->path.lineTo(x, y + height - dheight / 2);
        this->path.moveTo(x + width, y + dheight / 2);
        this->path.lineTo(x + width, y + height - dheight / 2);
        this->path.closeSubpath();
    }
    Object* Database::clone()
    {
        return new Database(this);
    }
    QString Database::graphicsName()
    {
        return "Database";
    }
    float Document::t = 0;
    Document::Document(int x, int y, int width, int height, float t)
    {
        nodes.push_back(new Node(QPointF(x, y + height / 2), this));
        nodes.push_back(new Node(QPointF(x + width / 2, y), this));
        nodes.push_back(new Node(QPointF(x + width / 2, y + height), this));
        nodes.push_back(new Node(QPointF(x + width, y + height / 2), this));
        Document::t = t;
        this->setZValue(1);

        this->setShowRect(QRectF(x, y, width, height));
        int dheight = height * t;
        this->path.moveTo(x, y + height - dheight);
        this->path.lineTo(x, y);
        this->path.lineTo(x + width, y);
        this->path.lineTo(x + width, y + height - dheight);
        this->path.arcMoveTo(QRectF(x, y + height - dheight * 2, width / 2, dheight * 2), 180);
        this->path.arcTo(QRectF(x, y + height - dheight * 2, width / 2, dheight * 2), 180, 180);
        this->path.arcMoveTo(QRectF(x + width / 2, y + height - dheight * 2, width / 2, dheight * 2), 0);
        this->path.arcTo(QRectF(x + width / 2, y + height - dheight * 2, width / 2, dheight * 2), 0, 180);
    }
    Document::Document(const Document* t):Object(t)
    {
    }
    void Document::graphics_Update(int x, int y, int width, int height)
    {
        nodes[0]->setPos(QPointF(x, y + height / 2));
        nodes[1]->setPos(QPointF(x + width / 2, y));
        nodes[2]->setPos(QPointF(x + width / 2, y + height));
        nodes[3]->setPos(QPointF(x + width, y + height / 2));
        this->setShowRect(QRectF(x, y, width, height));
        this->path.clear();
        int dheight = height * t;
        this->path.moveTo(x, y + height - dheight);
        this->path.lineTo(x, y);
        this->path.lineTo(x + width, y);
        this->path.lineTo(x + width, y + height - dheight);
        this->path.arcMoveTo(QRectF(x, y + height - dheight * 2, width / 2, dheight * 2), 180);
        this->path.arcTo(QRectF(x, y + height - dheight * 2, width / 2, dheight * 2), 180, 180);
        this->path.arcMoveTo(QRectF(x + width / 2, y + height - dheight * 2, width / 2, dheight * 2), 0);
        this->path.arcTo(QRectF(x + width / 2, y + height - dheight * 2, width / 2, dheight * 2), 0, 180);
    }
    Object* Document::clone()
    {
        return new Document(this);
    }
    QString Document::graphicsName()
    {
        return "Document";
    }
    int DataStorage::dwidth = 0;
    DataStorage::DataStorage(int x, int y, int width, int height, int dwidth)
    {
        nodes.push_back(new Node(QPointF(x, y + height / 2), this));
        nodes.push_back(new Node(QPointF(x + width / 2, y), this));
        nodes.push_back(new Node(QPointF(x + width / 2, y + height), this));
        nodes.push_back(new Node(QPointF(x + width, y + height / 2), this));
        DataStorage::dwidth = dwidth;
        this->setZValue(1);

        this->setShowRect(QRectF(x, y, width, height));
        this->path.arcMoveTo(QRectF(x, y, dwidth * 2, height), 90);
        this->path.arcTo(QRectF(x, y, dwidth * 2, height), 90, 180);
        this->path.moveTo(x + dwidth, y + height);
        this->path.lineTo(x + width, y + height);
        this->path.arcMoveTo(QRectF(x + width - dwidth, y, dwidth * 2, height), 90);
        this->path.arcTo(QRectF(x + width - dwidth, y, dwidth * 2, height), 90, 180);
        this->path.moveTo(x + width, y);
        this->path.lineTo(x + dwidth, y);
        this->path.closeSubpath();
    }
    DataStorage::DataStorage(const DataStorage* t):Object(t)
    {
    }
    void DataStorage::graphics_Update(int x, int y, int width, int height)
    {
        nodes[0]->setPos(QPointF(x, y + height / 2));
        nodes[1]->setPos(QPointF(x + width / 2, y));
        nodes[2]->setPos(QPointF(x + width / 2, y + height));
        nodes[3]->setPos(QPointF(x + width, y + height / 2));
        this->setShowRect(QRectF(x, y, width, height));
        this->path.clear();
        this->path.arcMoveTo(QRectF(x, y, dwidth * 2, height), 90);
        this->path.arcTo(QRectF(x, y, dwidth * 2, height), 90, 180);
        this->path.moveTo(x + dwidth, y + height);
        this->path.lineTo(x + width, y + height);
        this->path.arcMoveTo(QRectF(x + width - dwidth, y, dwidth * 2, height), 90);
        this->path.arcTo(QRectF(x + width - dwidth, y, dwidth * 2, height), 90, 180);
        this->path.moveTo(x + width, y);
        this->path.lineTo(x + dwidth, y);
        this->path.closeSubpath();
    }
    Object* DataStorage::clone()
    {
        return new DataStorage(this);
    }
    QString DataStorage::graphicsName()
    {
        return "DataStorage";
    }
    Object::Node::Node(QPointF p, Object* parent):parent(parent),point(p)
    {
        this->rect = QRectF(p.x() - width, p.y() - width, width * 2, width * 2);
    }
    Object::Node::Node(const Node& p, Object* parent):parent(parent)
    {
        this->point = p.point;
        this->rect = p.rect;
    }
    void Object::Node::addConnect(Node* connect)
    {
        this->connects.insert(connect);
    }
    void Object::Node::clear()
    {
        QSet<Node*>().swap(this->connects);
        QSet<Object*>().swap(this->vecHead);
        QSet<Object*>().swap(this->vecTail);
    }
    void Object::Node::removeConnect()
    {
        for (auto* it : this->connects) {
            for (auto* t : it->parent->nodes) {
                t->vecHead.remove(this->parent);
                t->vecTail.remove(this->parent);
            }
            
            it->connects.remove(this);
        }
        
        this->clear();
    }
    void Object::Node::setPos(QPointF p)
    {
        this->point = p;
        this->rect = QRectF(p.x() - width, p.y() - width, width * 2, width * 2);
    }
    void Object::Node::setRectF(QRectF r)
    {
        this->rect = r;
    }

    void Object::Node::AllowLine_Update()
    {
        for (auto* it : this->vecHead) {
            QPointF newpos = this->parent->mapRectToItem(it, this->rect).center();
            it->pos_Update(newpos.x(), newpos.y(), it->nodes[1]->point.x(), it->nodes[1]->point.y());
        }
        for (auto* it : this->vecTail) {
            QPointF newpos = this->parent->mapRectToItem(it, this->rect).center();
            it->pos_Update(it->nodes[0]->point.x(), it->nodes[0]->point.y(), newpos.x(),newpos.y());
        }
    }

    bool Object::Node::operator<(const Node* t)
    {
        return false;
    }

}
namespace base {
GraphicalManager* GraphicalManager::unique = nullptr;
GraphicalManager::GraphicalManager() {

}
GraphicalManager* GraphicalManager::getInstance() {
    if (!unique) {
        unique = new GraphicalManager();
    }
    return unique;
}
void GraphicalManager::clear()
{
    for (auto*& it : unique->graphics) {
        view->scene()->removeItem(it);
        delete it;
    }
    unique->graphics.clear();
}
}
namespace base {
ElementManager* ElementManager::unique = nullptr;

PushButton::PushButton(QString textname,QWidget* parent = nullptr) : QPushButton(parent) {
    this->setText(textname);
}


void PushButton::mousePressEvent(QMouseEvent* event) {
	dragStartPosition = event->pos();
	QPushButton::mousePressEvent(event);
}

void PushButton::mouseMoveEvent(QMouseEvent* event) {
	if (event->buttons() & Qt::LeftButton) {
		// 检查拖动距离  
		if ((event->pos() - dragStartPosition).manhattanLength() >= QApplication::startDragDistance()) {
			// 开始拖动  
			QDrag* drag = new QDrag(this);
			QMimeData* mimeData = new QMimeData;

			// 设置拖动数据  
			mimeData->setText(this->text()); // 可以设置其他数据  
			drag->setMimeData(mimeData);
			drag->setPixmap(icon().pixmap(size()));
			drag->setHotSpot(event->pos() - rect().topLeft());

			drag->exec(Qt::MoveAction);
		}
	}
	QPushButton::mouseMoveEvent(event);
}
ElementManager::ElementManager() {

}
ElementManager* ElementManager::getInstance() {
    if (!unique) {
        unique = new ElementManager();
    }
    return unique;
}
}
