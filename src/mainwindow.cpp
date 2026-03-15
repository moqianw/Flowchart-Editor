#include "mainwindow.h"

#include "ui_mainwindow.h"

#include "palettebutton.h"
#include "mermaidexporter.h"

#include <QAbstractSpinBox>
#include <QAction>
#include <QCheckBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollArea>
#include <QStatusBar>
#include <QToolButton>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_session(new flowchart::EditorSession(this))
    , m_view(new flowchart::CanvasView(m_session, this)) {
    ui->setupUi(this);
    ui->toolBox->setMinimumWidth(220);
    ui->toolBox->setMaximumWidth(260);
    ui->horizontalLayout_5->setStretch(0, 0);
    ui->horizontalLayout_5->setStretch(1, 1);
    ui->gridLayout->setContentsMargins(0, 0, 0, 0);
    ui->gridLayout->addWidget(m_view);

    populatePalette();
    setupWorkspaceEnhancements();
    bindUi();
    updateWindowTitle();
    updateInspector();
    updateStatusSummary();
}

bool MainWindow::maybeSave() {
    if (!m_session->isDirty()) {
        return true;
    }

    const QMessageBox::StandardButton result = QMessageBox::warning(
        this,
        QStringLiteral("Flowchart Editor"),
        QStringLiteral("文档已修改，是否先保存？"),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    if (result == QMessageBox::Cancel) {
        return false;
    }
    if (result == QMessageBox::Yes) {
        if (m_currentFile.isEmpty()) {
            const QString fileName = QFileDialog::getSaveFileName(
                this,
                QStringLiteral("保存流程图"),
                QString(),
                QStringLiteral("Flowchart (*.json)"));
            if (fileName.isEmpty()) {
                return false;
            }
            return saveToFile(fileName);
        }
        return saveToFile(m_currentFile);
    }

    return true;
}

bool MainWindow::saveToFile(const QString& fileName) {
    QString error;
    if (!m_session->saveToFile(fileName, &error)) {
        QMessageBox::critical(this, QStringLiteral("保存失败"), error);
        return false;
    }

    m_currentFile = fileName;
    m_session->markClean();
    updateWindowTitle();
    return true;
}

void MainWindow::updateWindowTitle() {
    const QString documentName = m_currentFile.isEmpty()
        ? QStringLiteral("Untitled")
        : QFileInfo(m_currentFile).fileName();
    setWindowTitle(QStringLiteral("%1%2 - Flowchart Editor")
        .arg(documentName)
        .arg(m_session->isDirty() ? QStringLiteral("*") : QString()));
}

void MainWindow::populatePalette() {
    if (!m_paletteLayout) {
        auto* page = ui->toolBox->widget(0);
        if (!page || !ui->formLayout) {
            return;
        }

        ui->formLayout->setContentsMargins(0, 0, 0, 0);
        ui->formLayout->setSpacing(0);

        auto* scrollArea = new QScrollArea(page);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        auto* paletteContent = new QWidget(scrollArea);
        m_paletteLayout = new QVBoxLayout(paletteContent);
        m_paletteLayout->setContentsMargins(0, 0, 0, 0);
        m_paletteLayout->setSpacing(6);
        m_paletteLayout->addStretch();
        paletteContent->setLayout(m_paletteLayout);

        scrollArea->setWidget(paletteContent);
        ui->formLayout->addRow(scrollArea);
    }

    for (const flowchart::ShapeDefinition* definition : m_session->registry().paletteDefinitions()) {
        auto* button = new flowchart::PaletteButton(definition->typeId, definition->paletteLabel, this);
        button->setIcon(QIcon(definition->iconPath));
        button->setIconSize(QSize(36, 36));
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setStyleSheet(QStringLiteral("text-align: left; padding: 6px 8px;"));
        button->setMinimumHeight(54);
        button->setToolTip(definition->typeId);
        connect(button, &QPushButton::clicked, this, [this, typeId = definition->typeId]() {
            m_session->createItem(typeId, m_view->sceneCenter());
        });

        if (m_paletteLayout) {
            m_paletteLayout->insertWidget(m_paletteLayout->count() - 1, button);
        }
    }
}

void MainWindow::setupWorkspaceEnhancements() {
    setupInspectorDock();
    setupAdvancedActions();

    m_statusSummaryLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_statusSummaryLabel, 1);
}

void MainWindow::setupInspectorDock() {
    m_inspectorDock = new QDockWidget(QStringLiteral("属性"), this);
    m_inspectorDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto* content = new QWidget(m_inspectorDock);
    auto* formLayout = new QFormLayout(content);
    formLayout->setContentsMargins(10, 10, 10, 10);
    formLayout->setSpacing(8);

    m_itemTypeLabel = new QLabel(QStringLiteral("-"), content);
    m_itemIdLabel = new QLabel(QStringLiteral("-"), content);
    m_itemIdLabel->setWordWrap(true);
    m_textEdit = new QLineEdit(content);
    m_xSpin = new QDoubleSpinBox(content);
    m_ySpin = new QDoubleSpinBox(content);
    m_widthSpin = new QDoubleSpinBox(content);
    m_heightSpin = new QDoubleSpinBox(content);
    m_rotationSpin = new QDoubleSpinBox(content);
    m_scaleSpin = new QDoubleSpinBox(content);
    m_strokeWidthSpin = new QSpinBox(content);

    const QList<QDoubleSpinBox*> geometrySpins = {
        m_xSpin,
        m_ySpin,
        m_widthSpin,
        m_heightSpin,
        m_rotationSpin,
        m_scaleSpin,
    };
    for (QDoubleSpinBox* spin : geometrySpins) {
        spin->setRange(-10000.0, 10000.0);
        spin->setDecimals(1);
        spin->setSingleStep(10.0);
    }
    m_widthSpin->setRange(20.0, 4000.0);
    m_heightSpin->setRange(20.0, 4000.0);
    m_rotationSpin->setRange(-360.0, 360.0);
    m_rotationSpin->setSingleStep(15.0);
    m_scaleSpin->setRange(0.2, 8.0);
    m_scaleSpin->setSingleStep(0.1);
    m_scaleSpin->setDecimals(2);
    m_strokeWidthSpin->setRange(1, 32);

    formLayout->addRow(QStringLiteral("类型"), m_itemTypeLabel);
    formLayout->addRow(QStringLiteral("ID"), m_itemIdLabel);
    formLayout->addRow(QStringLiteral("文本"), m_textEdit);
    formLayout->addRow(QStringLiteral("X"), m_xSpin);
    formLayout->addRow(QStringLiteral("Y"), m_ySpin);
    formLayout->addRow(QStringLiteral("宽度"), m_widthSpin);
    formLayout->addRow(QStringLiteral("高度"), m_heightSpin);
    formLayout->addRow(QStringLiteral("旋转"), m_rotationSpin);
    formLayout->addRow(QStringLiteral("缩放"), m_scaleSpin);
    formLayout->addRow(QStringLiteral("线宽"), m_strokeWidthSpin);

    content->setLayout(formLayout);
    m_inspectorDock->setWidget(content);
    addDockWidget(Qt::RightDockWidgetArea, m_inspectorDock);

    const auto applyGeometry = [this]() {
        if (!m_syncingInspector) {
            applyInspectorGeometry();
        }
    };
    connect(m_textEdit, &QLineEdit::editingFinished, this, [this]() {
        if (m_syncingInspector) {
            return;
        }
        const QString itemId = currentSingleSelectedItemId();
        if (!itemId.isEmpty()) {
            m_session->setText(QStringList{itemId}, m_textEdit->text());
        }
    });
    connect(m_xSpin, &QAbstractSpinBox::editingFinished, this, applyGeometry);
    connect(m_ySpin, &QAbstractSpinBox::editingFinished, this, applyGeometry);
    connect(m_widthSpin, &QAbstractSpinBox::editingFinished, this, applyGeometry);
    connect(m_heightSpin, &QAbstractSpinBox::editingFinished, this, applyGeometry);
    connect(m_rotationSpin, &QAbstractSpinBox::editingFinished, this, applyGeometry);
    connect(m_scaleSpin, &QAbstractSpinBox::editingFinished, this, applyGeometry);
    connect(m_strokeWidthSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        if (m_syncingInspector || !m_session->hasSingleSelection()) {
            return;
        }
        Q_UNUSED(value);
        m_session->applyStrokeWidth(m_strokeWidthSpin->value());
    });
}

void MainWindow::setupAdvancedActions() {
    auto* toolsMenu = menuBar()->addMenu(QStringLiteral("工具"));
    m_autoLayoutAction = new QAction(QStringLiteral("自动布局"), this);
    m_autoLayoutAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+L")));
    m_validateAction = new QAction(QStringLiteral("校验流程图"), this);
    m_validateAction->setShortcut(QKeySequence(QStringLiteral("F7")));
    m_snapToGridAction = new QAction(QStringLiteral("吸附到网格"), this);
    m_snapToGridAction->setCheckable(true);
    m_snapToGridAction->setChecked(m_session->snapToGridEnabled());
    m_exportMermaidAction = new QAction(QStringLiteral("Mermaid"), this);

    ui->menu_4->addAction(m_exportMermaidAction);
    toolsMenu->addAction(m_autoLayoutAction);
    toolsMenu->addAction(m_validateAction);
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_snapToGridAction);
}

void MainWindow::updateInspector() {
    m_syncingInspector = true;

    const QString itemId = currentSingleSelectedItemId();
    const auto* item = itemId.isEmpty() ? nullptr : m_session->document().item(itemId);
    const auto* node = itemId.isEmpty() ? nullptr : m_session->document().node(itemId);

    const bool hasSingle = item != nullptr;
    const bool isNode = node != nullptr;

    m_itemTypeLabel->setText(hasSingle ? item->typeId : QStringLiteral("未选择"));
    m_itemIdLabel->setText(hasSingle ? item->id : QStringLiteral("-"));
    m_textEdit->setEnabled(hasSingle);
    m_strokeWidthSpin->setEnabled(hasSingle);
    m_xSpin->setEnabled(isNode);
    m_ySpin->setEnabled(isNode);
    m_widthSpin->setEnabled(isNode);
    m_heightSpin->setEnabled(isNode);
    m_rotationSpin->setEnabled(isNode);
    m_scaleSpin->setEnabled(isNode);

    if (!hasSingle) {
        m_textEdit->clear();
        m_strokeWidthSpin->setValue(1);
        m_xSpin->setValue(0.0);
        m_ySpin->setValue(0.0);
        m_widthSpin->setValue(0.0);
        m_heightSpin->setValue(0.0);
        m_rotationSpin->setValue(0.0);
        m_scaleSpin->setValue(1.0);
        m_syncingInspector = false;
        return;
    }

    m_textEdit->setText(item->text);
    m_strokeWidthSpin->setValue(item->style.strokeWidth);

    if (isNode) {
        m_xSpin->setValue(node->rect.x());
        m_ySpin->setValue(node->rect.y());
        m_widthSpin->setValue(node->rect.width());
        m_heightSpin->setValue(node->rect.height());
        m_rotationSpin->setValue(node->style.rotation);
        m_scaleSpin->setValue(node->style.scale);
    } else {
        m_xSpin->setValue(0.0);
        m_ySpin->setValue(0.0);
        m_widthSpin->setValue(0.0);
        m_heightSpin->setValue(0.0);
        m_rotationSpin->setValue(0.0);
        m_scaleSpin->setValue(1.0);
    }

    m_syncingInspector = false;
}

void MainWindow::updateStatusSummary() {
    int nodeCount = 0;
    int connectorCount = 0;
    for (const QString& itemId : m_session->document().itemIds()) {
        if (m_session->document().node(itemId)) {
            ++nodeCount;
        } else if (m_session->document().connector(itemId)) {
            ++connectorCount;
        }
    }

    const QStringList issues = m_session->validateDocument();
    const QString validationState = issues.isEmpty()
        ? QStringLiteral("校验通过")
        : QStringLiteral("警告 %1").arg(issues.size());

    if (m_statusSummaryLabel) {
        m_statusSummaryLabel->setText(QStringLiteral(
            "节点 %1 | 连线 %2 | 选中 %3 | 网格吸附 %4 | %5")
                .arg(nodeCount)
                .arg(connectorCount)
                .arg(m_session->selectedItemIds().size())
                .arg(m_session->snapToGridEnabled() ? QStringLiteral("开") : QStringLiteral("关"))
                .arg(validationState));
    }
}

void MainWindow::applyInspectorGeometry() {
    const QString itemId = currentSingleSelectedItemId();
    const auto* node = itemId.isEmpty() ? nullptr : m_session->document().node(itemId);
    if (!node) {
        return;
    }

    const QRectF rect(
        m_xSpin->value(),
        m_ySpin->value(),
        m_widthSpin->value(),
        m_heightSpin->value());
    m_session->updateNodeGeometry(
        itemId,
        rect,
        m_rotationSpin->value(),
        m_scaleSpin->value());
}

QString MainWindow::currentSingleSelectedItemId() const {
    return m_session->hasSingleSelection() ? m_session->primarySelectedItemId() : QString();
}

void MainWindow::bindUi() {
    connect(m_session, &flowchart::EditorSession::dirtyChanged, this, [this](bool) {
        updateWindowTitle();
    });
    connect(m_session, &flowchart::EditorSession::selectionChanged, this, [this]() {
        updateInspector();
        updateStatusSummary();
    });
    connect(m_session, &flowchart::EditorSession::documentChanged, this, [this]() {
        updateInspector();
        updateStatusSummary();
    });

    connect(ui->fontComboBox, &QFontComboBox::currentFontChanged, this, [this](const QFont& font) {
        m_session->applyFontFamily(font);
    });
    connect(ui->comboBox, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        m_session->applyFontSize(text.toInt());
    });

    connect(ui->toolButton_4, &QToolButton::clicked, this, [this]() { m_session->toggleBold(); });
    connect(ui->toolButton_5, &QToolButton::clicked, this, [this]() { m_session->toggleItalic(); });
    connect(ui->toolButton_6, &QToolButton::clicked, this, [this]() { m_session->toggleUnderline(); });

    connect(ui->copyAction, &QAction::triggered, this, [this]() { m_session->copySelected(); });
    connect(ui->pasteAction, &QAction::triggered, this, [this]() { m_session->paste(m_view->sceneCenter()); });
    connect(ui->cutAction, &QAction::triggered, this, [this]() { m_session->cutSelected(); });
    connect(ui->allAction, &QAction::triggered, this, [this]() { m_session->selectAll(); });
    connect(ui->revertAction, &QAction::triggered, this, [this]() { m_session->undoStack()->undo(); });
    connect(ui->action_2, &QAction::triggered, this, [this]() { m_session->undoStack()->redo(); });

    connect(ui->comboBox_4, &QComboBox::currentIndexChanged, this, [this](int index) {
        static const QVector<QColor> colors = {
            QColor(255, 255, 255, 255),
            QColor(0, 0, 0, 255),
            QColor(0, 0, 255, 255),
            QColor(255, 0, 0, 255),
            QColor(255, 255, 102, 255),
            QColor(0, 255, 0, 255),
        };

        if (index >= 0 && index < colors.size()) {
            m_session->applyStrokeColor(colors[index]);
            return;
        }
        if (index == colors.size()) {
            const QColor color = QColorDialog::getColor(QColor(0, 0, 0, 255), this, QStringLiteral("选择边框颜色"));
            if (color.isValid()) {
                m_session->applyStrokeColor(color);
            }
        }
    });

    connect(ui->comboBox_2, &QComboBox::currentIndexChanged, this, [this](int index) {
        static const QVector<QColor> colors = {
            QColor(0, 0, 0, 255),
            QColor(0, 0, 255, 255),
            QColor(255, 0, 0, 255),
            QColor(255, 255, 102, 255),
            QColor(0, 255, 0, 255),
        };

        if (index >= 0 && index < colors.size()) {
            m_session->applyFillColor(colors[index]);
            return;
        }
        if (index == colors.size()) {
            const QColor color = QColorDialog::getColor(QColor(255, 255, 255, 0), this, QStringLiteral("选择填充颜色"));
            if (color.isValid()) {
                m_session->applyFillColor(color);
            }
        }
    });

    connect(ui->comboBox_3, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        m_session->applyStrokeWidth(text.toInt());
    });

    connect(ui->toolButton_3, &QToolButton::clicked, this, [this]() { m_session->deleteSelected(); });
    connect(ui->toolButton_2, &QToolButton::clicked, this, [this]() { m_session->bringForward(); });
    connect(ui->toolButton, &QToolButton::clicked, this, [this]() { m_session->sendBackward(); });

    connect(ui->enlargeAction, &QAction::triggered, this, [this]() { m_view->zoomInView(); });
    connect(ui->lessenAction, &QAction::triggered, this, [this]() { m_view->zoomOutView(); });
    connect(ui->leftAction, &QAction::triggered, this, [this]() { m_view->rotateViewLeft(); });
    connect(ui->rightAction, &QAction::triggered, this, [this]() { m_view->rotateViewRight(); });
    connect(ui->toolButton_17, &QToolButton::clicked, this, [this]() { m_session->scaleSelected(1.15); });
    connect(ui->toolButton_18, &QToolButton::clicked, this, [this]() { m_session->scaleSelected(1.0 / 1.15); });
    connect(ui->toolButton_19, &QToolButton::clicked, this, [this]() { m_session->rotateSelected(-45.0); });
    connect(ui->toolButton_20, &QToolButton::clicked, this, [this]() { m_session->rotateSelected(45.0); });

    connect(ui->newAction, &QAction::triggered, this, [this]() {
        if (!maybeSave()) {
            return;
        }
        m_currentFile.clear();
        m_session->resetDocument();
        updateWindowTitle();
    });

    connect(ui->openAction, &QAction::triggered, this, [this]() {
        if (!maybeSave()) {
            return;
        }
        const QString fileName = QFileDialog::getOpenFileName(
            this,
            QStringLiteral("打开流程图"),
            QString(),
            QStringLiteral("Flowchart (*.json)"));
        if (fileName.isEmpty()) {
            return;
        }

        QString error;
        if (!m_session->loadFromFile(fileName, &error)) {
            QMessageBox::critical(this, QStringLiteral("打开失败"), error);
            return;
        }

        m_currentFile = fileName;
        updateWindowTitle();
    });

    auto saveCurrent = [this]() {
        if (m_currentFile.isEmpty()) {
            const QString fileName = QFileDialog::getSaveFileName(
                this,
                QStringLiteral("保存流程图"),
                QString(),
                QStringLiteral("Flowchart (*.json)"));
            if (fileName.isEmpty()) {
                return false;
            }
            return saveToFile(fileName);
        }
        return saveToFile(m_currentFile);
    };

    connect(ui->action, &QAction::triggered, this, saveCurrent);
    connect(ui->saveAction, &QAction::triggered, this, saveCurrent);

    connect(ui->closeaction, &QAction::triggered, this, [this]() {
        if (!maybeSave()) {
            return;
        }
        m_currentFile.clear();
        m_session->resetDocument();
        updateWindowTitle();
    });

    connect(ui->action_4, &QAction::triggered, this, [this, saveCurrent]() {
        if (!saveCurrent()) {
            return;
        }
        m_currentFile.clear();
        m_session->resetDocument();
        updateWindowTitle();
    });

    connect(ui->actionsvg, &QAction::triggered, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, QStringLiteral("导出 SVG"), QString(), QStringLiteral("SVG (*.svg)"));
        if (fileName.isEmpty()) {
            return;
        }
        if (QFileInfo(fileName).suffix().isEmpty()) {
            fileName += QStringLiteral(".svg");
        }
        m_view->exportToSvg(fileName);
    });

    connect(ui->actionpng, &QAction::triggered, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, QStringLiteral("导出 PNG"), QString(), QStringLiteral("PNG (*.png)"));
        if (fileName.isEmpty()) {
            return;
        }
        if (QFileInfo(fileName).suffix().isEmpty()) {
            fileName += QStringLiteral(".png");
        }
        m_view->exportToPng(fileName);
    });

    connect(m_autoLayoutAction, &QAction::triggered, this, [this]() {
        m_session->autoLayoutSelection();
    });
    connect(m_validateAction, &QAction::triggered, this, [this]() {
        const QStringList issues = m_session->validateDocument();
        if (issues.isEmpty()) {
            QMessageBox::information(this, QStringLiteral("流程图校验"), QStringLiteral("校验通过，未发现结构问题。"));
            return;
        }
        QMessageBox::warning(this, QStringLiteral("流程图校验"), issues.join(QLatin1Char('\n')));
    });
    connect(m_snapToGridAction, &QAction::toggled, this, [this](bool checked) {
        m_session->setSnapToGridEnabled(checked);
        updateStatusSummary();
    });
    connect(m_exportMermaidAction, &QAction::triggered, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(
            this,
            QStringLiteral("导出 Mermaid"),
            QString(),
            QStringLiteral("Mermaid (*.mmd *.mermaid *.txt)"));
        if (fileName.isEmpty()) {
            return;
        }
        if (QFileInfo(fileName).suffix().isEmpty()) {
            fileName += QStringLiteral(".mmd");
        }

        QString error;
        if (!flowchart::MermaidExporter::save(m_session->document(), fileName, &error)) {
            QMessageBox::critical(this, QStringLiteral("导出失败"), error);
        }
    });
}

MainWindow::~MainWindow() {
    delete ui;
}
