#ifndef BASE_H
#define BASE_H
#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsItem>
#include <QMainWindow>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <QMenu>
#include <QMessageBox>
#include <QObject>
#include <QPixmap>
#include <QLabel>
#include <QDrag>
#include <QMimeData>
#include <QGraphicsTextItem>
#include <QTextEdit>
#include <QSvgGenerator>
#include <QSvgRenderer>
#include <QPushButton>
#include <QDragEnterEvent>  
#include <QDropEvent>  
#include <QDataStream>

namespace base {

//基类
class Object :public QObject, public QGraphicsItem {
    Q_OBJECT
public:

    QPainterPath path;//路径

    QRectF showrectf;//显示区域
    QRectF rectf;//真实区域
    int rectfwidth;//边界区域

    QColor color;//边框颜色rgba
    QBrush brush;//填充颜色rgba
    int penWidth;//边框粗细
    bool operator<(const Object* t);
    //右键菜单栏，选项栏
    static QMenu* menu;
    Object(const Object* t);
    //文本框
    //加粗
    bool bold_checked = false;
    void setFontBold(bool bold);
    //斜体
    bool italic_checked = false;
    void setFontItalic(bool italic) {
        QFont font = textItem->font();
        font.setItalic(italic);
        textItem->setFont(font);
        scene()->addItem(textItem);
    }
    //下划线
    bool underline_checked = false;
    void setFontUnderline(bool underline) {
        QFont font = textItem->font();
        font.setUnderline(underline);
        textItem->setFont(font);
        scene()->addItem(textItem);
    }

    QRectF boundingRect() const override;
    void setRect(QRectF rect);

    void setShowRect(QRectF rect);

    void setWidth(int width);
    void setColor(QColor color);
    //
    //节点
    class Node {
    public:
        QSet<Node*> connects;
        Object* parent = nullptr;
        qreal width = 10;
        QPointF point;
        QRectF rect;
        QSet<Object*> vecHead;
        QSet<Object*> vecTail;
        Node(QPointF p, Object* parent);
        Node(const Node& p, Object* parent);
        void addConnect(Node* connect);
        void clear();
        void removeConnect();
        void setPos(QPointF p);
        void setRectF(QRectF r);
        void AllowLine_Update();
        bool operator<(const Node* t);
    };
    QVector<Node*> nodes;

    
    Object();
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    QGraphicsTextItem* textItem = nullptr;
    //拷贝
    virtual Object* clone();
    //文件保存读取
    virtual QString graphicsName();
    virtual void save(QDataStream& out);
    ~Object();

private:

    void updateTextItemPosition();

protected:

    bool ifrecord = false;//判断是否记录
    int resize = -1;
    QPointF mouselastscenePos;
    QPointF mouselastPos;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    virtual void graphics_Update(int x1, int y1, int x2, int y2);
    virtual void pos_Update(int x1, int y1, int x2, int y2);

    int getPosition(QGraphicsSceneMouseEvent* event);
public slots:
    void setBrush(QBrush brush);
};
//边框
class OptionBox :public Object {
    OptionBox(Object* parent);
    static OptionBox* unique;
public:

    QVector<Object*> parents;
    static OptionBox* getInstance(Object* parent);
    static OptionBox* getInstance();
    void addObject(Object* parent);
    void clear();
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*);
};
//椭圆
class Ellipse :public Object {
public:
    Ellipse(int x, int y, int width, int height);
    Ellipse(const Ellipse* t);
    void graphics_Update(int x, int y, int width, int height) override;
    inline Object* clone() override;
    QString graphicsName() override;
};
//矩形
class Rectangle :public Object {
public:
    Rectangle(int x, int y, int width, int height);
    Rectangle(const Rectangle* t);
    void graphics_Update(int x, int y, int width, int height) override;
    inline Object* clone() override;
    QString graphicsName() override;
};
//圆角矩形
class RoundedRectangle :public Object {
    static int rx;
    static int ry;
public:
    //x,y,width,height,rx,ry
    RoundedRectangle(int x, int y, int width, int height ,int rx,int ry);
    RoundedRectangle(const RoundedRectangle* t);
    void graphics_Update(int x, int y, int width, int height) override;
    inline Object* clone() override;
    QString graphicsName() override;
};
//菱形
class Diamond :public Object {
public:
    Diamond(int x, int y, int width, int height);
    Diamond(const Diamond* t);
    void graphics_Update(int x, int y, int width, int height) override;
    inline Object* clone() override;
    QString graphicsName() override;
};
//平行四边形
class Parallelogram :public Object {
    static float t;//左下角角度
public:
    Parallelogram(int x, int y, int width, int height, float t);
    Parallelogram(const Parallelogram* t);
    void graphics_Update(int x, int y, int width, int height) override;
    inline Object* clone() override;
    QString graphicsName() override;
};
//开始与结束
class Start_or_Terminator :public Object {
public:
    Start_or_Terminator(int x, int y, int width, int height);
    Start_or_Terminator(const Start_or_Terminator* t);
    void graphics_Update(int x, int y, int width, int height) override;
    inline Object* clone() override;
    QString graphicsName() override;
};
//子流程
class Subprocess :public Object {
    static int dwidth;
public:
    Subprocess(int x, int y, int width, int height,int dwidth);
    Subprocess(const Subprocess* t);
    void graphics_Update(int x, int y, int width, int height) override;
    inline Object* clone() override;
    QString graphicsName() override;
};
//数据库
class Database :public Object {
    static float t;
public:
    Database(int x, int y, int width, int height, float t);
    Database(const Database* t);
    void graphics_Update(int x, int y, int width, int height) override;
    inline Object* clone() override;
    QString graphicsName() override;
};
//文件
class Document :public Object {
    static float t;
public:
    Document(int x, int y, int width, int height, float t);
    Document(const Document* t);
    void graphics_Update(int x, int y, int width, int height) override;
    inline Object* clone() override;
    QString graphicsName() override;
};
//数据存储
class DataStorage :public Object {
    static int dwidth;
public:
    DataStorage(int x, int y, int width, int height, int dwidth);
    DataStorage(const DataStorage* t);
    void graphics_Update(int x, int y, int width, int height) override;
    inline Object* clone() override;
    QString graphicsName() override;
};
//带箭头直线
class AllowLine :public Object {
public:

    void createArrowHead(QPointF start, QPointF end, QPainterPath& path);//绘制箭头

    AllowLine(int x1, int y1, int x2, int y2);
    AllowLine(const AllowLine* t);
    ~AllowLine();
    void graphics_Update(int x, int y, int width, int height) override;
    void pos_Update(int x1, int y1, int x2, int y2) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    inline Object* clone() override;
    QString graphicsName() override;
    void save(QDataStream& out) override;
};

//文本框
class Textpointer :public Object {
public:
    Textpointer(QGraphicsScene* scene, int x, int y, int width, int height);
    Textpointer(const Textpointer* t);
    void graphics_Update(int x, int y, int width, int height) override;
    Object* clone() override;
    QString graphicsName() override;
};
//图形管理器
class GraphicalManager {
    GraphicalManager();
    static GraphicalManager* unique;
public:
    QGraphicsView* view = nullptr;
    QVector<Object*> graphics;
    static GraphicalManager* getInstance();
    void clear();
};
}
namespace base {

class PushButton : public QPushButton {
    Q_OBJECT

public:
    PushButton(QString textname,QWidget* parent);

protected:
    void mousePressEvent(QMouseEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QPoint dragStartPosition;
};

class CustomGraphicsView : public QGraphicsView {
    Q_OBJECT

public:
    CustomGraphicsView(QGraphicsScene* scene, QWidget* parent = nullptr)
        : QGraphicsView(scene, parent) {
        setAcceptDrops(true); // 允许视图接受拖放事件  
    }

protected:
    void dragEnterEvent(QDragEnterEvent* event) override {
        if (event->mimeData()->hasText()) {
            event->acceptProposedAction();
        }
    }

    void dropEvent(QDropEvent* event) override {
        if (event->mimeData()->hasText()) {
            QPointF scenePos = mapToScene(event->pos());
            // 创建 QGraphicsItem  
            QGraphicsEllipseItem* item = scene()->addEllipse(scenePos.x(), scenePos.y(), 50, 50, QPen(Qt::black), QBrush(Qt::blue));
            qDebug() << "Created item at:" << scenePos;
            event->acceptProposedAction();
        }
    }
};
//元件管理器
class ElementManager {
    ElementManager();
    static ElementManager* unique;
public:
    std::vector<PushButton*> elements;
    static ElementManager* getInstance();
};
}
#endif // BASE_H
