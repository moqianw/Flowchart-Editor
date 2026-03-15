#include "mainwindow.h"

#include "ui_mainwindow.h"

#include "palettebutton.h"
#include "mermaidexporter.h"
#include "sessionpersistence.h"

#include <QAbstractSpinBox>
#include <QAction>
#include <QCheckBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QKeySequence>
#include <QListWidgetItem>
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

    populatePalette();
    setupWorkspaceEnhancements();
    setupPersistence();
    bindUi();
    restoreRecoverySnapshotIfAvailable();
    updateWindowTitle();
    updateInspector();
    updateStatusSummary();
}

bool MainWindow::maybeSave() {
    if (!m_hasActiveDocument || !m_session->isDirty()) {
        return true;
    }

    const QString documentName = m_currentFile.isEmpty()
        ? QStringLiteral("未命名文档")
        : QFileInfo(m_currentFile).fileName();

    const QMessageBox::StandardButton result = QMessageBox::warning(
        this,
        QStringLiteral("保存更改"),
        QStringLiteral("文档“%1”已修改，是否在继续前保存？").arg(documentName),
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
    m_hasActiveDocument = true;
    flowchart::SessionPersistence::rememberRecentFile(fileName);
    updateRecentFilesMenu();
    updateRecentFilesWelcomeList();
    m_session->markClean();
    discardRecoverySnapshot();
    updateDocumentAvailability();
    updateWindowTitle();
    return true;
}

bool MainWindow::openFromFile(const QString& fileName, bool rememberRecent) {
    QString error;
    if (!m_session->loadFromFile(fileName, &error)) {
        QMessageBox::critical(this, QStringLiteral("打开失败"), error);
        flowchart::SessionPersistence::removeRecentFile(fileName);
        updateRecentFilesMenu();
        updateRecentFilesWelcomeList();
        return false;
    }

    m_currentFile = fileName;
    m_hasActiveDocument = true;
    if (rememberRecent) {
        flowchart::SessionPersistence::rememberRecentFile(fileName);
    }
    updateRecentFilesMenu();
    updateRecentFilesWelcomeList();
    discardRecoverySnapshot();
    updateDocumentAvailability();
    updateWindowTitle();
    return true;
}

void MainWindow::updateWindowTitle() {
    if (!m_hasActiveDocument) {
        setWindowTitle(QStringLiteral("Flowchart Editor"));
        return;
    }

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
        m_paletteButtons.push_back(button);
    }
}

void MainWindow::setupWorkspaceEnhancements() {
    setupWelcomeView();
    setupInspectorDock();
    setupAdvancedActions();

    m_statusSummaryLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_statusSummaryLabel, 1);
}

void MainWindow::setupWelcomeView() {
    m_workspaceStack = new QStackedWidget(this);
    m_workspaceStack->setContentsMargins(0, 0, 0, 0);

    m_welcomeView = new QWidget(m_workspaceStack);
    auto* layout = new QVBoxLayout(m_welcomeView);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(14);

    auto* title = new QLabel(QStringLiteral("Flowchart Editor"), m_welcomeView);
    QFont titleFont = title->font();
    titleFont.setPointSize(22);
    titleFont.setBold(true);
    title->setFont(titleFont);

    auto* subtitle = new QLabel(
        QStringLiteral("创建新文件或打开最近使用的文件后，才会进入画布编辑。"),
        m_welcomeView);
    subtitle->setWordWrap(true);
    subtitle->setStyleSheet(QStringLiteral("color: #666;"));

    auto* newButton = new QPushButton(QStringLiteral("新建文件"), m_welcomeView);
    auto* openButton = new QPushButton(QStringLiteral("打开文件"), m_welcomeView);
    newButton->setMinimumHeight(40);
    openButton->setMinimumHeight(40);

    auto* recentTitle = new QLabel(QStringLiteral("最近使用"), m_welcomeView);
    QFont recentFont = recentTitle->font();
    recentFont.setBold(true);
    recentTitle->setFont(recentFont);

    m_recentFilesEmptyLabel = new QLabel(QStringLiteral("暂无最近文件"), m_welcomeView);
    m_recentFilesEmptyLabel->setStyleSheet(QStringLiteral("color: #888;"));
    m_recentFilesList = new QListWidget(m_welcomeView);
    m_recentFilesList->setAlternatingRowColors(true);
    m_recentFilesList->setUniformItemSizes(true);

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(newButton);
    layout->addWidget(openButton);
    layout->addSpacing(8);
    layout->addWidget(recentTitle);
    layout->addWidget(m_recentFilesEmptyLabel);
    layout->addWidget(m_recentFilesList, 1);

    connect(newButton, &QPushButton::clicked, this, [this]() { startNewDocument(); });
    connect(openButton, &QPushButton::clicked, this, [this]() { openDocumentDialog(); });
    connect(m_recentFilesList, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) {
        if (!item || !maybeSave()) {
            return;
        }
        const QString fileName = item->data(Qt::UserRole).toString();
        if (!fileName.isEmpty()) {
            openFromFile(fileName);
        }
    });

    m_workspaceStack->addWidget(m_welcomeView);
    m_workspaceStack->addWidget(m_view);
    ui->gridLayout->addWidget(m_workspaceStack);
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
    m_fillAlphaSpin = new QSpinBox(content);
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
    m_fillAlphaSpin->setRange(0, 255);
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
    formLayout->addRow(QStringLiteral("填充透明度"), m_fillAlphaSpin);
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
    connect(m_fillAlphaSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        if (m_syncingInspector || !m_session->hasSingleSelection()) {
            return;
        }
        m_session->applyFillAlpha(value);
    });
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
    m_recentFilesMenu = new QMenu(QStringLiteral("最近文件"), this);
    m_autoLayoutAction = new QAction(QStringLiteral("自动布局"), this);
    m_autoLayoutAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+L")));
    m_validateAction = new QAction(QStringLiteral("校验流程图"), this);
    m_validateAction->setShortcut(QKeySequence(QStringLiteral("F7")));
    m_snapToGridAction = new QAction(QStringLiteral("吸附到网格"), this);
    m_snapToGridAction->setCheckable(true);
    m_snapToGridAction->setChecked(m_session->snapToGridEnabled());
    m_exportMermaidAction = new QAction(QStringLiteral("Mermaid"), this);

    ui->menu->insertMenu(ui->action, m_recentFilesMenu);
    ui->menu_4->addAction(m_exportMermaidAction);
    if (m_inspectorDock) {
        m_inspectorDock->toggleViewAction()->setText(QStringLiteral("属性面板"));
        toolsMenu->addAction(m_inspectorDock->toggleViewAction());
        toolsMenu->addSeparator();
    }
    toolsMenu->addAction(m_autoLayoutAction);
    toolsMenu->addAction(m_validateAction);
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_snapToGridAction);
}

void MainWindow::setupPersistence() {
    updateRecentFilesMenu();
    updateRecentFilesWelcomeList();
    updateDocumentAvailability();

    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(15000);
    connect(m_autoSaveTimer, &QTimer::timeout, this, [this]() {
        if (!m_session->isDirty()) {
            return;
        }
        persistRecoverySnapshot();
    });
    m_autoSaveTimer->start();
}

void MainWindow::updateDocumentAvailability() {
    if (m_workspaceStack && m_welcomeView) {
        m_workspaceStack->setCurrentWidget(m_hasActiveDocument ? static_cast<QWidget*>(m_view) : m_welcomeView);
    }

    for (QPushButton* button : std::as_const(m_paletteButtons)) {
        if (button) {
            button->setEnabled(m_hasActiveDocument);
        }
    }

    const QList<QWidget*> widgets = {
        ui->fontComboBox,
        ui->comboBox,
        ui->comboBox_2,
        ui->comboBox_3,
        ui->comboBox_4,
        ui->toolButton,
        ui->toolButton_2,
        ui->toolButton_3,
        ui->toolButton_4,
        ui->toolButton_5,
        ui->toolButton_6,
        ui->toolButton_17,
        ui->toolButton_18,
        ui->toolButton_19,
        ui->toolButton_20,
    };
    for (QWidget* widget : widgets) {
        if (widget) {
            widget->setEnabled(m_hasActiveDocument);
        }
    }

    const QList<QAction*> actions = {
        ui->saveAction,
        ui->action,
        ui->closeaction,
        ui->action_4,
        ui->copyAction,
        ui->pasteAction,
        ui->cutAction,
        ui->revertAction,
        ui->allAction,
        ui->action_2,
        ui->enlargeAction,
        ui->lessenAction,
        ui->leftAction,
        ui->rightAction,
        ui->actionsvg,
        ui->actionpng,
        m_autoLayoutAction,
        m_validateAction,
        m_snapToGridAction,
        m_exportMermaidAction,
    };
    for (QAction* action : actions) {
        if (action) {
            action->setEnabled(m_hasActiveDocument);
        }
    }

    if (m_inspectorDock) {
        m_inspectorDock->setEnabled(m_hasActiveDocument);
    }
}

void MainWindow::updateInspector() {
    m_syncingInspector = true;

    if (!m_hasActiveDocument) {
        m_itemTypeLabel->setText(QStringLiteral("未打开文档"));
        m_itemIdLabel->setText(QStringLiteral("-"));
        m_textEdit->clear();
        m_textEdit->setEnabled(false);
        m_fillAlphaSpin->setEnabled(false);
        m_strokeWidthSpin->setEnabled(false);
        m_xSpin->setEnabled(false);
        m_ySpin->setEnabled(false);
        m_widthSpin->setEnabled(false);
        m_heightSpin->setEnabled(false);
        m_rotationSpin->setEnabled(false);
        m_scaleSpin->setEnabled(false);
        m_syncingInspector = false;
        return;
    }

    const QString itemId = currentSingleSelectedItemId();
    const auto* item = itemId.isEmpty() ? nullptr : m_session->document().item(itemId);
    const auto* node = itemId.isEmpty() ? nullptr : m_session->document().node(itemId);

    const bool hasSingle = item != nullptr;
    const bool isNode = node != nullptr;

    m_itemTypeLabel->setText(hasSingle ? item->typeId : QStringLiteral("未选择"));
    m_itemIdLabel->setText(hasSingle ? item->id : QStringLiteral("-"));
    m_textEdit->setEnabled(hasSingle);
    m_fillAlphaSpin->setEnabled(hasSingle);
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
        m_fillAlphaSpin->setValue(0);
        m_syncingInspector = false;
        return;
    }

    m_textEdit->setText(item->text);
    m_fillAlphaSpin->setValue(item->style.fillColor.alpha());
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
    if (!m_hasActiveDocument) {
        const int recentCount = flowchart::SessionPersistence::recentFiles().size();
        if (m_statusSummaryLabel) {
            m_statusSummaryLabel->setText(QStringLiteral("未打开文档 | 最近文件 %1").arg(recentCount));
        }
        return;
    }

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
        if (!m_session->isDirty()) {
            discardRecoverySnapshot();
        }
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
            QColor initialColor(255, 255, 255, 0);
            const QString itemId = currentSingleSelectedItemId();
            if (!itemId.isEmpty()) {
                if (const auto* item = m_session->document().item(itemId)) {
                    initialColor = item->style.fillColor;
                }
            }
            const QColor color = QColorDialog::getColor(
                initialColor,
                this,
                QStringLiteral("选择填充颜色"),
                QColorDialog::ShowAlphaChannel);
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

    connect(ui->newAction, &QAction::triggered, this, [this]() { startNewDocument(); });
    connect(ui->openAction, &QAction::triggered, this, [this]() { openDocumentDialog(); });

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
        discardRecoverySnapshot();
        m_hasActiveDocument = false;
        updateDocumentAvailability();
        updateWindowTitle();
    });

    connect(ui->action_4, &QAction::triggered, this, [this, saveCurrent]() {
        if (!saveCurrent()) {
            return;
        }
        m_currentFile.clear();
        m_session->resetDocument();
        discardRecoverySnapshot();
        m_hasActiveDocument = false;
        updateDocumentAvailability();
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

void MainWindow::updateRecentFilesMenu() {
    if (!m_recentFilesMenu) {
        return;
    }

    m_recentFilesMenu->clear();
    const QStringList recentFiles = flowchart::SessionPersistence::recentFiles();
    if (recentFiles.isEmpty()) {
        QAction* placeholder = m_recentFilesMenu->addAction(QStringLiteral("无最近文件"));
        placeholder->setEnabled(false);
        return;
    }

    for (const QString& fileName : recentFiles) {
        const QFileInfo info(fileName);
        QAction* action = m_recentFilesMenu->addAction(info.fileName());
        action->setToolTip(fileName);
        connect(action, &QAction::triggered, this, [this, fileName]() {
            if (!maybeSave()) {
                return;
            }
            openFromFile(fileName);
        });
    }
}

void MainWindow::updateRecentFilesWelcomeList() {
    if (!m_recentFilesList || !m_recentFilesEmptyLabel) {
        return;
    }

    m_recentFilesList->clear();
    const QStringList recentFiles = flowchart::SessionPersistence::recentFiles();
    m_recentFilesEmptyLabel->setVisible(recentFiles.isEmpty());
    m_recentFilesList->setVisible(!recentFiles.isEmpty());

    for (const QString& fileName : recentFiles) {
        const QFileInfo info(fileName);
        auto* item = new QListWidgetItem(info.fileName(), m_recentFilesList);
        item->setToolTip(fileName);
        item->setData(Qt::UserRole, fileName);
    }
}

void MainWindow::startNewDocument() {
    if (!maybeSave()) {
        return;
    }

    m_currentFile.clear();
    m_session->resetDocument();
    discardRecoverySnapshot();
    m_hasActiveDocument = true;
    updateDocumentAvailability();
    updateWindowTitle();
}

void MainWindow::openDocumentDialog() {
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
    openFromFile(fileName);
}

void MainWindow::persistRecoverySnapshot() {
    if (!m_hasActiveDocument) {
        return;
    }
    QString error;
    auto snapshot = m_session->exportDocumentSnapshot();
    if (!flowchart::SessionPersistence::saveRecoverySnapshot(snapshot, m_currentFile, &error)) {
        statusBar()->showMessage(QStringLiteral("自动保存失败：%1").arg(error), 5000);
    }
}

void MainWindow::discardRecoverySnapshot() {
    flowchart::SessionPersistence::discardRecoverySnapshot();
}

void MainWindow::restoreRecoverySnapshotIfAvailable() {
    if (!flowchart::SessionPersistence::hasRecoverySnapshot()) {
        return;
    }

    QString error;
    flowchart::RecoverySnapshot snapshot = flowchart::SessionPersistence::loadRecoverySnapshot(&error);
    if (!error.isEmpty() || !snapshot.isValid()) {
        discardRecoverySnapshot();
        return;
    }

    QString detail;
    if (!snapshot.sourceFile.isEmpty()) {
        detail = QStringLiteral("来源文件：%1").arg(snapshot.sourceFile);
    }
    if (snapshot.savedAt.isValid()) {
        if (!detail.isEmpty()) {
            detail += QLatin1Char('\n');
        }
        detail += QStringLiteral("自动保存时间：%1").arg(snapshot.savedAt.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    }

    const QMessageBox::StandardButton choice = QMessageBox::question(
        this,
        QStringLiteral("恢复上次内容"),
        detail.isEmpty()
            ? QStringLiteral("检测到上次未正常退出时的自动保存内容，是否恢复？")
            : QStringLiteral("检测到上次未正常退出时的自动保存内容，是否恢复？\n\n%1").arg(detail),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);

    if (choice != QMessageBox::Yes) {
        discardRecoverySnapshot();
        return;
    }

    m_currentFile = snapshot.sourceFile;
    m_session->restoreRecoveredDocument(std::move(snapshot.items));
    m_hasActiveDocument = true;
    updateDocumentAvailability();
    updateWindowTitle();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!maybeSave()) {
        event->ignore();
        return;
    }

    discardRecoverySnapshot();
    event->accept();
}

MainWindow::~MainWindow() {
    delete ui;
}
