#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , scene(new QGraphicsScene(this))
    , view(new base::GraphicsView(scene, this))
{
    view->main = this;
    base::GraphicalManager* manager = base::GraphicalManager::getInstance();
    if (!manager->view) manager->view = view;
    ui->setupUi(this);


    //QWidget* centerWidget = ui->centralwidget;//



    scene->setSceneRect(0,0,800,600);
    ui->gridLayout->addWidget(view);
    //
    set_Labels();
    add_BaseElement();
    //git config --global http.proxy
}



void MainWindow::set_Labels()
{

    //设置字体
    connect(ui->fontComboBox, &QFontComboBox::currentFontChanged, [=](const QFont& font) {
        base::OptionBox* box = base::OptionBox::getInstance();
        for (auto* it : box->parents) {
            if (it->textItem)
                it->textItem->setFont(font);
        }
        });
    //设置字体大小
    connect(ui->comboBox, &QComboBox::currentTextChanged, [=](const QString& text) {
        base::OptionBox* box = base::OptionBox::getInstance();
        int fontSize = text.toInt();
        for (auto* it : box->parents) {
            if (it->textItem) {
                QFont font = it->textItem->font(); // 获取当前字体
                font.setPointSize(fontSize); // 设置新的字体大小
                it->textItem->setFont(font); // 应用新的字体
            }

        }
        });
    //设置加粗
    connect(ui->toolButton_4, &QToolButton::clicked, [=]() {
        base::OptionBox* box = base::OptionBox::getInstance();
        for (auto* it : box->parents) {
            if (it->textItem) {
                it->bold_checked = !it->bold_checked;
                it->setFontBold(it->bold_checked);
            }

        }
        });
    //设置斜体
    connect(ui->toolButton_5, &QToolButton::clicked, [=]() {
        base::OptionBox* box = base::OptionBox::getInstance();
        for (auto* it : box->parents) {
            if (it->textItem) {
                it->italic_checked = !it->italic_checked;
                it->setFontItalic(it->italic_checked);
            }

        }
        });
    //设置下划线
    connect(ui->toolButton_6, &QToolButton::clicked, [=]() {
        base::OptionBox* box = base::OptionBox::getInstance();




        for (auto* it : box->parents) {
            if (it->textItem) {
                it->underline_checked = !it->underline_checked;
                it->setFontUnderline(it->underline_checked);
            }

        }
        });
    //复制
    connect(ui->copyAction, &QAction::triggered, this, [this]() {
        base::OptionBox* box = base::OptionBox::getInstance();
        base::CopyManager* copym = base::CopyManager::getInstance();
        copym->clear();
        for (auto* it : box->parents) {
            copym->add(it);
        }
        });
    //粘贴
    connect(ui->pasteAction, &QAction::triggered, this, [this]() {
        base::CopyManager* copym = base::CopyManager::getInstance();
        QVector<base::Object*> newobj;
        if (!copym->childs.isEmpty()) {
            base::GraphicalManager* manager = base::GraphicalManager::getInstance();
            for (auto* it : copym->childs) {
                QPointF lastPos = it->mapRectFromParent(it->rectf).topLeft();
                QPointF dpos = lastPos - mouseNowPos;
                manager->graphics.push_back(it->clone());
                newobj.push_back(manager->graphics.back());
                manager->view->scene()->addItem(manager->graphics.back());
            }
            base::OperationRecord* record = base::OperationRecord::getInstance();

            record->recodeCommand(newobj, base::OperationRecord::Create);

        }
        });
    //剪切
    connect(ui->cutAction, &QAction::triggered, this, [this]() {
        base::OptionBox* box = base::OptionBox::getInstance();
        base::CopyManager* copym = base::CopyManager::getInstance();
        copym->clear();
        for (auto* it : box->parents) {
            copym->add(it);
        }
        base::GraphicalManager* manager = base::GraphicalManager::getInstance();

        base::OperationRecord* record = base::OperationRecord::getInstance();
        record->recodeCommand(box->parents, base::OperationRecord::Delete);

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
    //全选
    connect(ui->allAction, &QAction::triggered, this, [this]() {
        base::OptionBox* box = base::OptionBox::getInstance();
        for (auto* it : base::GraphicalManager::getInstance()->graphics) {
            box->addObject(it);
        }

        });
    //撤销
    connect(ui->revertAction, &QAction::triggered, this, [this]() {
        base::OperationRecord::getInstance()->cancelCommand();
        });
    //导出为svg
    connect(ui->actionsvg, &QAction::triggered, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(view,
            "保存图片",
            "",
            "SVG (*.svg)");

        // 如果用户取消保存操作，则 fileName 为空
        if (fileName.isEmpty()) {
            return;
        }

        // 获取用户选择的文件格式
        QString format = QFileInfo(fileName).suffix().toUpper();
        if (format.isEmpty()) {
            fileName += ".svg";
        }
        view->toSvg(fileName);
        });
    //导出为png
    connect(ui->actionpng, &QAction::triggered, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(view,
            "保存图片",
            "",
            "Images (*.png *.jpg *.bmp)");

        // 如果用户取消保存操作，则 fileName 为空
        if (fileName.isEmpty()) {
            return;
        }

        // 获取用户选择的文件格式
        QString format = QFileInfo(fileName).suffix().toUpper();
        if (format.isEmpty()) {
            fileName += ".png";
        }
        view->toPng(fileName);
        });
    //边框颜色
    connect(ui->comboBox_4, &QComboBox::currentIndexChanged, this, [this](int index) {
        base::OptionBox* box = base::OptionBox::getInstance();

        base::OperationRecord* record = base::OperationRecord::getInstance();
        record->recodeCommand(box->parents, base::OperationRecord::Trans);
        for (auto* it : box->parents) {
            if (index == 0) {
                it->setColor(QColor(255, 255, 255, 255));
            }
            else if (index == 1) {
                it->setColor(QColor(0, 0, 0, 255));
            }
            else if (index == 2) {
                it->setColor(QColor(0, 0, 255, 255));
            }
            else if (index == 3) {
                it->setColor(QColor(255, 0, 0, 255));
            }
            else if (index == 4) {
                it->setColor(QColor(255, 255, 102, 255));
            }
            else if (index == 5) {
                it->setColor(QColor(0, 255, 0, 255));
            }
        }
        if (index == 6) {

			QColor color = QColorDialog::getColor(QColor(0,0,0,0), this, "Select Color");
			if (color.isValid()) {
                for (auto* it : box->parents) {
                    it->setColor(color);
                }
			}
        }

        });
    //填充颜色
    connect(ui->comboBox_2, &QComboBox::currentIndexChanged, this, [this](int index) {
        base::OptionBox* box = base::OptionBox::getInstance();
        base::OperationRecord* record = base::OperationRecord::getInstance();
        record->recodeCommand(box->parents, base::OperationRecord::Trans);
        for (auto* it : box->parents) {
            if (index == 0) {
                it->setBrush(QColor(0, 0, 0, 255));
            }
            else if (index == 1) {
                it->setBrush(QColor(0, 0, 255, 255));
            }
            else if (index == 2) {
                it->setBrush(QColor(255, 0, 0, 255));
            }
            else if (index == 3) {
                it->setBrush(QColor(255, 255, 102, 255));
            }
            else if (index == 4) {
                it->setBrush(QColor(0, 255, 0, 255));
            }
        }
        if (index == 5) {

            QColor color = QColorDialog::getColor(QColor(0, 0, 0, 0), this, "Select Brush");
            if (color.isValid()) {
                for (auto* it : box->parents) {
                    it->setBrush(color);
                }
            }
        }

        });//
    //删除
    connect(ui->toolButton_3, &QToolButton::clicked, this, [this]() {
        base::OptionBox* box = base::OptionBox::getInstance();
        base::GraphicalManager* manager = base::GraphicalManager::getInstance();

        base::OperationRecord* record = base::OperationRecord::getInstance();

        record->recodeCommand(box->parents, base::OperationRecord::Delete);
        for (auto*& it : box->parents) {
            manager->view->scene()->removeItem(it);
            auto t = std::find(manager->graphics.begin(), manager->graphics.end(), it);

            if (t != manager->graphics.end()) {
                manager->graphics.erase(t);
                delete it;
                it = nullptr;
            }
        }
        box->clear();
        });
    //向上移
    connect(ui->toolButton_2, &QToolButton::clicked, this, [this]() {
        base::OptionBox* box = base::OptionBox::getInstance();
        for (auto* it : box->parents) {
            it->setZValue(it->zValue() + 1);
        }
        });
    //向下移
    connect(ui->toolButton, &QToolButton::clicked, this, [this]() {
        base::OptionBox* box = base::OptionBox::getInstance();
        for (auto* it : box->parents) {
            it->setZValue(it->zValue() - 1);
        }
        });
    //边框粗细
    connect(ui->comboBox_3, &QComboBox::currentTextChanged, this, [=](const QString& text) {
        base::OptionBox* box = base::OptionBox::getInstance();
        base::OperationRecord* record = base::OperationRecord::getInstance();
        record->recodeCommand(box->parents, base::OperationRecord::Trans);
        int size = text.toInt();
        for (auto* it : box->parents) {
            it->setWidth(size);
        }
        });
    //整体旋转,缩放
    connect(ui->enlargeAction, &QAction::triggered, this, [=]() {
        view->zoomOut();
        });
    connect(ui->lessenAction, &QAction::triggered, this, [=]() {
        view->zoomIn();
        });
    connect(ui->leftAction, &QAction::triggered, this, [=]() {
        view->rotateLeft();
        });
    connect(ui->rightAction, &QAction::triggered, this, [=]() {
        view->rotateright();
        });
    connect(ui->toolButton_17, &QToolButton::clicked, this, [=]() {
        view->scaleSelectedItem(1.15);
        });
    connect(ui->toolButton_18, &QToolButton::clicked, this, [=]() {
        view->scaleSelectedItem(1.0 / 1.15);
        });
    connect(ui->toolButton_19, &QToolButton::clicked, this, [=]() {
        view->rotateSelectedItem(-45);
        });
    connect(ui->toolButton_20, &QToolButton::clicked, this, [=]() {
        view->rotateSelectedItem(45);
        });
    //新建
    connect(ui->newAction, &QAction::triggered, this, [=]() {
        if (!filename.isEmpty())  classStream::save(base::GraphicalManager::getInstance()->graphics, filename);
        filename = QFileDialog::getSaveFileName(nullptr, "Save File", "", "BIN(*.bin);;All Files (*)");

        if (filename.isEmpty()) {
            qDebug() << "No file selected.";
            return; // 用户取消了保存操作
        }

        });
    //关闭
    connect(ui->closeaction, &QAction::triggered, this, [=]() {
        filename.clear();
        base::OperationRecord::getInstance()->clear();
        base::GraphicalManager::getInstance()->clear();

        });
    //保存并关闭
    connect(ui->action_4, &QAction::triggered, this, [=]() {
        if(filename.isEmpty())  filename = QFileDialog::getOpenFileName(nullptr, "Save File", "", "BIN(*.bin);;All Files (*)");
        if (filename.isEmpty()) {
            qDebug() << "No file selected.";
        }
        else {
            classStream::save(base::GraphicalManager::getInstance()->graphics, filename);
        }
        QString().swap(filename);
        base::GraphicalManager::getInstance()->clear();
        });
    //保存
    connect(ui->action, &QAction::triggered, this, [=]() {
        if (filename.isEmpty()) filename = QFileDialog::getOpenFileName(nullptr, "Save File", "", "BIN(*.bin);;All Files (*)");
        if (filename.isEmpty()) {
            qDebug() << "No file selected.";
            return;
        }
        classStream::save(base::GraphicalManager::getInstance()->graphics, filename);
        });
    //打开
    connect(ui->openAction, &QAction::triggered, this, [=]() {
        classStream::save(base::GraphicalManager::getInstance()->graphics, filename);
        filename = QFileDialog::getOpenFileName(nullptr, "Save File", "", "BIN(*.bin);;All Files (*)");

        if (filename.isEmpty()) {
            qDebug() << "No file selected.";
            return; // 用户取消了保存操作
        }
        QVector<base::Object*>& t =classStream::load(filename);
        base::GraphicalManager* manager = base::GraphicalManager::getInstance();
        for (auto* it : t) {
            manager->graphics.push_back(it);
            manager->view->scene()->addItem(manager->graphics.back());
        }
        
        });
}

void MainWindow::add_BaseElement()
{
    base::ElementManager* manager = base::ElementManager::getInstance();
    manager->elements.push_back(new base::PushButton("Elliesp", this));
    manager->elements.push_back(new base::PushButton("Rectangle", this));
    manager->elements.push_back(new base::PushButton("Diamond", this));
    manager->elements.push_back(new base::PushButton("RoundedRectangle", this));
    manager->elements.push_back(new base::PushButton("Parallelogram", this));
    manager->elements.push_back(new base::PushButton("Start_or_Terminator", this));
    manager->elements.push_back(new base::PushButton("Subprocess", this));
    manager->elements.push_back(new base::PushButton("Database", this));
    manager->elements.push_back(new base::PushButton("Document", this));
    manager->elements.push_back(new base::PushButton("DataStorage", this));
    manager->elements.push_back(new base::PushButton("Textpointer", this));
    manager->elements.push_back(new base::PushButton("Allowline", this));
    
    for (int i = 1; i <= manager->elements.size(); i++) {
        ui->formLayout->addRow(manager->elements[i - 1]);
        manager->elements[i - 1]->setIcon(QPixmap(QString(":/image/")+manager->elements[i - 1]->text()+QString(".png")));
        manager->elements[i - 1]->setIconSize(QSize(48, 48));
        //左对齐
        manager->elements[i - 1]->setStyleSheet("text-align: left;");
        //manager->elements[i - 1]->setStyleSheet("QPushButton { color: transparent; }");
    }
    this->setGeometry(0, 0, 1280, 760);
}

void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    this->mouseNowPos = event->pos();
    QMainWindow::mouseMoveEvent(event);
}

MainWindow::~MainWindow()
{
    delete ui;
}
