#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "base.h"
#include <QMainWindow>
#include <QEvent>
#include "QTimer"
#include "view.h"
#include <QToolBar>
#include <QColorDialog>
#include <QToolButton>
#include <QFileDialog>
#include "graphcopy.h"
#include "OperationRecord.h"
#include "classStream.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);

    ~MainWindow();
    Ui::MainWindow* ui;
private:
    QString filename;
    QPointF mouseNowPos;
    QTimer* timer;
    QGraphicsScene* scene;
    base::GraphicsView* view;
    void set_Labels();
    void add_BaseElement();
    void mouseMoveEvent(QMouseEvent* event) override;
};
#endif // MAINWINDOW_H
