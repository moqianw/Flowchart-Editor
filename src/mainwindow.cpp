#include "mainwindow.h"

#include "ui_mainwindow.h"

#include "componentextensionmanager.h"
#include "customcomponentdialog.h"
#include "palettebutton.h"
#include "mermaidexporter.h"
#include "sessionpersistence.h"

#include <QAbstractSpinBox>
#include <QAction>
#include <QCheckBox>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QKeySequence>
#include <QListWidgetItem>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStatusBar>
#include <QTextDocument>
#include <QToolButton>
#include <QUrl>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_session(new flowchart::EditorSession(this))
    , m_collaborationManager(new flowchart::PeerCollaborationManager(m_session, this))
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

    qDeleteAll(m_paletteButtons);
    m_paletteButtons.clear();

    for (const flowchart::ShapeDefinition* definition : m_session->registry().paletteDefinitions()) {
        auto* button = new flowchart::PaletteButton(definition->typeId, definition->paletteLabel, this);
        button->setIcon(definition->paletteIcon.isNull() ? QIcon(definition->iconPath) : definition->paletteIcon);
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

void MainWindow::refreshPalette() {
    populatePalette();
    updateDocumentAvailability();
}

void MainWindow::setupWorkspaceEnhancements() {
    setupWelcomeView();
    setupInspectorDock();
    setupComponentsDock();
    setupCollaborationDock();
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

void MainWindow::setupComponentsDock() {
    m_componentsDock = new QDockWidget(QStringLiteral("组件管理"), this);
    m_componentsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto* content = new QWidget(m_componentsDock);
    auto* rootLayout = new QVBoxLayout(content);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(8);

    auto* topButtonsRow = new QWidget(content);
    auto* topButtonsLayout = new QHBoxLayout(topButtonsRow);
    topButtonsLayout->setContentsMargins(0, 0, 0, 0);
    topButtonsLayout->setSpacing(6);

    auto* newButton = new QPushButton(QStringLiteral("新建"), topButtonsRow);
    auto* importButton = new QPushButton(QStringLiteral("导入"), topButtonsRow);
    auto* refreshButton = new QPushButton(QStringLiteral("刷新"), topButtonsRow);
    auto* openDirButton = new QPushButton(QStringLiteral("目录"), topButtonsRow);
    topButtonsLayout->addWidget(newButton);
    topButtonsLayout->addWidget(importButton);
    topButtonsLayout->addWidget(refreshButton);
    topButtonsLayout->addWidget(openDirButton);

    auto* splitter = new QSplitter(Qt::Vertical, content);
    m_componentsList = new QListWidget(splitter);
    m_componentsList->setAlternatingRowColors(true);
    m_componentsList->setUniformItemSizes(true);

    auto* editorPane = new QWidget(splitter);
    auto* editorLayout = new QVBoxLayout(editorPane);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(6);

    auto* detailsLayout = new QFormLayout();
    detailsLayout->setContentsMargins(0, 0, 0, 0);
    detailsLayout->setSpacing(6);
    m_componentTypeValueLabel = new QLabel(QStringLiteral("-"), editorPane);
    m_componentLabelValueLabel = new QLabel(QStringLiteral("-"), editorPane);
    m_componentFileValueLabel = new QLabel(QStringLiteral("-"), editorPane);
    m_componentFileValueLabel->setWordWrap(true);
    detailsLayout->addRow(QStringLiteral("类型"), m_componentTypeValueLabel);
    detailsLayout->addRow(QStringLiteral("名称"), m_componentLabelValueLabel);
    detailsLayout->addRow(QStringLiteral("文件"), m_componentFileValueLabel);

    auto* editorHint = new QLabel(
        QStringLiteral("直接编辑组件定义 JSON。保存后会自动刷新图形栏和注册表。"),
        editorPane);
    editorHint->setWordWrap(true);
    editorHint->setStyleSheet(QStringLiteral("color: #666;"));

    m_componentJsonEdit = new QPlainTextEdit(editorPane);
    m_componentJsonEdit->setPlaceholderText(QStringLiteral("{\n  \"typeId\": \"CustomNode\",\n  \"paletteLabel\": \"自定义节点\",\n  ...\n}"));

    auto* bottomButtonsRow = new QWidget(editorPane);
    auto* bottomButtonsLayout = new QHBoxLayout(bottomButtonsRow);
    bottomButtonsLayout->setContentsMargins(0, 0, 0, 0);
    bottomButtonsLayout->setSpacing(6);
    m_componentSaveButton = new QPushButton(QStringLiteral("保存修改"), bottomButtonsRow);
    m_componentExportButton = new QPushButton(QStringLiteral("导出"), bottomButtonsRow);
    m_componentDeleteButton = new QPushButton(QStringLiteral("删除"), bottomButtonsRow);
    bottomButtonsLayout->addWidget(m_componentSaveButton);
    bottomButtonsLayout->addWidget(m_componentExportButton);
    bottomButtonsLayout->addWidget(m_componentDeleteButton);
    bottomButtonsLayout->addStretch(1);

    editorLayout->addLayout(detailsLayout);
    editorLayout->addWidget(editorHint);
    editorLayout->addWidget(m_componentJsonEdit, 1);
    editorLayout->addWidget(bottomButtonsRow);

    splitter->addWidget(m_componentsList);
    splitter->addWidget(editorPane);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    rootLayout->addWidget(topButtonsRow);
    rootLayout->addWidget(splitter, 1);

    m_componentsDock->setWidget(content);
    addDockWidget(Qt::RightDockWidgetArea, m_componentsDock);
    if (m_inspectorDock) {
        tabifyDockWidget(m_inspectorDock, m_componentsDock);
        m_inspectorDock->raise();
    }

    connect(newButton, &QPushButton::clicked, this, [this]() { createCustomComponent(); });
    connect(importButton, &QPushButton::clicked, this, [this]() { importComponentPack(); });
    connect(refreshButton, &QPushButton::clicked, this, [this]() { refreshComponentsPanel(); });
    connect(openDirButton, &QPushButton::clicked, this, [this]() { openComponentsDirectory(); });
    connect(m_componentsList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem*, QListWidgetItem*) {
        updateManagedComponentDetails();
    });
    connect(m_componentSaveButton, &QPushButton::clicked, this, [this]() { saveManagedComponentChanges(); });
    connect(m_componentExportButton, &QPushButton::clicked, this, [this]() { exportManagedComponent(); });
    connect(m_componentDeleteButton, &QPushButton::clicked, this, [this]() { deleteManagedComponent(); });

    refreshComponentsPanel();
}

void MainWindow::setupCollaborationDock() {
    m_collaborationDock = new QDockWidget(QStringLiteral("联机房间"), this);
    m_collaborationDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto* content = new QWidget(m_collaborationDock);
    auto* rootLayout = new QVBoxLayout(content);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(8);

    auto* formLayout = new QFormLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(6);

    m_collaborationNameEdit = new QLineEdit(content);
    m_collaborationNameEdit->setText(m_collaborationManager->localDisplayName());
    m_collaborationHostEdit = new QLineEdit(content);
    m_collaborationHostEdit->setPlaceholderText(QStringLiteral("例如 192.168.1.23"));
    m_collaborationPortSpin = new QSpinBox(content);
    m_collaborationPortSpin->setRange(1024, 65535);
    m_collaborationPortSpin->setValue(45454);
    m_collaborationStatusLabel = new QLabel(QStringLiteral("未开房"), content);
    m_collaborationPeerLabel = new QLabel(QStringLiteral("-"), content);

    formLayout->addRow(QStringLiteral("昵称"), m_collaborationNameEdit);
    formLayout->addRow(QStringLiteral("房主 IP"), m_collaborationHostEdit);
    formLayout->addRow(QStringLiteral("房间端口"), m_collaborationPortSpin);
    formLayout->addRow(QStringLiteral("房间状态"), m_collaborationStatusLabel);
    formLayout->addRow(QStringLiteral("已连接成员"), m_collaborationPeerLabel);

    auto* buttonsRow = new QWidget(content);
    auto* buttonsLayout = new QHBoxLayout(buttonsRow);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    buttonsLayout->setSpacing(6);
    m_collaborationHostButton = new QPushButton(QStringLiteral("房主开房"), buttonsRow);
    m_collaborationJoinButton = new QPushButton(QStringLiteral("加入房间"), buttonsRow);
    m_collaborationDisconnectButton = new QPushButton(QStringLiteral("退出房间"), buttonsRow);
    buttonsLayout->addWidget(m_collaborationHostButton);
    buttonsLayout->addWidget(m_collaborationJoinButton);
    buttonsLayout->addWidget(m_collaborationDisconnectButton);

    auto* hint = new QLabel(
        QStringLiteral("房主点击“房主开房”后，把自己的局域网 IP 和端口发给其他人。其他人输入房主 IP 和端口后即可直连加入。当前版本使用最后写入覆盖策略同步整张文档，适合局域网或可直连环境。"),
        content);
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("color: #666;"));

    m_collaborationLogEdit = new QPlainTextEdit(content);
    m_collaborationLogEdit->setReadOnly(true);
    m_collaborationLogEdit->setPlaceholderText(QStringLiteral("房间日志"));
    m_collaborationLogEdit->document()->setMaximumBlockCount(200);

    rootLayout->addLayout(formLayout);
    rootLayout->addWidget(buttonsRow);
    rootLayout->addWidget(hint);
    rootLayout->addWidget(m_collaborationLogEdit, 1);

    m_collaborationDock->setWidget(content);
    addDockWidget(Qt::RightDockWidgetArea, m_collaborationDock);
    if (m_componentsDock) {
        tabifyDockWidget(m_componentsDock, m_collaborationDock);
    } else if (m_inspectorDock) {
        tabifyDockWidget(m_inspectorDock, m_collaborationDock);
    }

    connect(m_collaborationHostButton, &QPushButton::clicked, this, [this]() { startHostingCollaboration(); });
    connect(m_collaborationJoinButton, &QPushButton::clicked, this, [this]() { joinPeerCollaboration(); });
    connect(m_collaborationDisconnectButton, &QPushButton::clicked, this, [this]() { disconnectCollaboration(); });
    connect(m_collaborationNameEdit, &QLineEdit::editingFinished, this, [this]() {
        if (m_collaborationManager) {
            m_collaborationManager->setLocalDisplayName(m_collaborationNameEdit->text());
        }
    });

    updateCollaborationUi();
}

void MainWindow::setupAdvancedActions() {
    auto* toolsMenu = menuBar()->addMenu(QStringLiteral("工具"));
    m_recentFilesMenu = new QMenu(QStringLiteral("最近文件"), this);
    m_connectorMenu = new QMenu(QStringLiteral("连接线"), this);
    m_autoLayoutAction = new QAction(QStringLiteral("自动布局"), this);
    m_autoLayoutAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+L")));
    m_validateAction = new QAction(QStringLiteral("校验流程图"), this);
    m_validateAction->setShortcut(QKeySequence(QStringLiteral("F7")));
    m_snapToGridAction = new QAction(QStringLiteral("吸附到网格"), this);
    m_snapToGridAction->setCheckable(true);
    m_snapToGridAction->setChecked(m_session->snapToGridEnabled());
    m_exportMermaidAction = new QAction(QStringLiteral("Mermaid"), this);
    m_importComponentsAction = new QAction(QStringLiteral("导入组件扩展包"), this);
    m_createComponentAction = new QAction(QStringLiteral("新建自定义组件"), this);
    m_openComponentsDirAction = new QAction(QStringLiteral("打开组件目录"), this);
    m_insertConnectorAction = new QAction(QStringLiteral("插入连接线"), this);
    m_insertConnectorAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+L")));
    m_resetConnectorBendAction = new QAction(QStringLiteral("自动路由"), this);
    m_routeConnectorVerticalAction = new QAction(QStringLiteral("纵向折线"), this);
    m_routeConnectorHorizontalAction = new QAction(QStringLiteral("横向折线"), this);

    ui->menu->insertMenu(ui->action, m_recentFilesMenu);
    ui->menu_4->addAction(m_exportMermaidAction);
    if (m_inspectorDock) {
        m_inspectorDock->toggleViewAction()->setText(QStringLiteral("属性面板"));
        toolsMenu->addAction(m_inspectorDock->toggleViewAction());
    }
    if (m_componentsDock) {
        m_componentsDock->toggleViewAction()->setText(QStringLiteral("组件管理"));
        toolsMenu->addAction(m_componentsDock->toggleViewAction());
    }
    if (m_collaborationDock) {
        m_collaborationDock->toggleViewAction()->setText(QStringLiteral("联机房间"));
        toolsMenu->addAction(m_collaborationDock->toggleViewAction());
    }
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_importComponentsAction);
    toolsMenu->addAction(m_createComponentAction);
    toolsMenu->addAction(m_openComponentsDirAction);
    toolsMenu->addSeparator();
    m_connectorMenu->addAction(m_insertConnectorAction);
    m_connectorMenu->addSeparator();
    m_connectorMenu->addAction(m_resetConnectorBendAction);
    m_connectorMenu->addAction(m_routeConnectorVerticalAction);
    m_connectorMenu->addAction(m_routeConnectorHorizontalAction);
    toolsMenu->addMenu(m_connectorMenu);
    toolsMenu->addSeparator();
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
        ui->connectorBarFrame,
        ui->connectorInsertButton,
        ui->connectorAutoButton,
        ui->connectorVerticalButton,
        ui->connectorHorizontalButton,
        ui->connectorHeadCombo,
        ui->connectorPatternCombo,
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
        m_insertConnectorAction,
        m_resetConnectorBendAction,
        m_routeConnectorVerticalAction,
        m_routeConnectorHorizontalAction,
    };
    for (QAction* action : actions) {
        if (action) {
            action->setEnabled(m_hasActiveDocument);
        }
    }

    if (m_inspectorDock) {
        m_inspectorDock->setEnabled(m_hasActiveDocument);
    }
    if (m_componentsDock) {
        m_componentsDock->setEnabled(true);
    }
    updateConnectorActions();
    updateCollaborationUi();
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
    const QString collaborationState = m_collaborationManager
        ? m_collaborationManager->statusText()
        : QStringLiteral("未连接");

    if (!m_hasActiveDocument) {
        const int recentCount = flowchart::SessionPersistence::recentFiles().size();
        if (m_statusSummaryLabel) {
            m_statusSummaryLabel->setText(
                QStringLiteral("未打开文档 | 最近文件 %1 | 协同 %2")
                    .arg(recentCount)
                    .arg(collaborationState));
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
            "节点 %1 | 连线 %2 | 选中 %3 | 网格吸附 %4 | %5 | 协同 %6")
                .arg(nodeCount)
                .arg(connectorCount)
                .arg(m_session->selectedItemIds().size())
                .arg(m_session->snapToGridEnabled() ? QStringLiteral("开") : QStringLiteral("关"))
                .arg(validationState)
                .arg(collaborationState));
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

void MainWindow::updateConnectorActions() {
    bool hasConnectorSelection = false;
    for (const QString& itemId : m_session->selectedItemIds()) {
        if (m_session->document().connector(itemId)) {
            hasConnectorSelection = true;
            break;
        }
    }

    if (m_resetConnectorBendAction) {
        m_resetConnectorBendAction->setEnabled(m_hasActiveDocument && hasConnectorSelection);
    }
    if (m_routeConnectorVerticalAction) {
        m_routeConnectorVerticalAction->setEnabled(m_hasActiveDocument && hasConnectorSelection);
    }
    if (m_routeConnectorHorizontalAction) {
        m_routeConnectorHorizontalAction->setEnabled(m_hasActiveDocument && hasConnectorSelection);
    }
    if (ui->connectorBarFrame) {
        ui->connectorBarFrame->setEnabled(m_hasActiveDocument);
    }

    const bool enableStyleControls = m_hasActiveDocument && hasConnectorSelection;
    ui->connectorAutoButton->setEnabled(enableStyleControls);
    ui->connectorVerticalButton->setEnabled(enableStyleControls);
    ui->connectorHorizontalButton->setEnabled(enableStyleControls);
    ui->connectorHeadCombo->setEnabled(enableStyleControls);
    ui->connectorPatternCombo->setEnabled(enableStyleControls);
    ui->connectorInsertButton->setEnabled(m_hasActiveDocument);

    QString selectedHeadStyle = QStringLiteral("stealth");
    QString selectedLineStyle = QStringLiteral("solid");
    for (const QString& itemId : m_session->selectedItemIds()) {
        const auto* connector = m_session->document().connector(itemId);
        if (!connector) {
            continue;
        }
        selectedHeadStyle = connector->props.value(QStringLiteral("headStyle")).toString(QStringLiteral("stealth"));
        selectedLineStyle = connector->props.value(QStringLiteral("lineStyle")).toString(QStringLiteral("solid"));
        break;
    }

    m_syncingConnectorControls = true;
    ui->connectorHeadCombo->setCurrentIndex(
        selectedHeadStyle == QStringLiteral("classic") ? 0
            : selectedHeadStyle == QStringLiteral("stealth") ? 1
            : selectedHeadStyle == QStringLiteral("diamond") ? 2
            : 3);
    ui->connectorPatternCombo->setCurrentIndex(
        selectedLineStyle == QStringLiteral("solid") ? 0
            : selectedLineStyle == QStringLiteral("dash") ? 1
            : 2);
    m_syncingConnectorControls = false;
}

void MainWindow::updateCollaborationUi() {
    if (!m_collaborationManager || !m_collaborationStatusLabel) {
        return;
    }

    const bool connected = m_collaborationManager->isConnected();
    const bool connecting = m_collaborationManager->isConnecting();
    const bool hosting = m_collaborationManager->isHosting();

    m_collaborationStatusLabel->setText(m_collaborationManager->statusText());
    m_collaborationPeerLabel->setText(m_collaborationManager->remoteDisplayName());

    if (m_collaborationNameEdit && m_collaborationNameEdit->text().trimmed() != m_collaborationManager->localDisplayName()) {
        m_collaborationNameEdit->setText(m_collaborationManager->localDisplayName());
    }

    if (m_collaborationPortSpin) {
        if (hosting && !connected && !connecting && m_collaborationManager->listeningPort() > 0) {
            m_collaborationPortSpin->setValue(m_collaborationManager->listeningPort());
        }
        m_collaborationPortSpin->setEnabled(!connected && !connecting && !hosting);
    }
    if (m_collaborationHostEdit) {
        m_collaborationHostEdit->setEnabled(!connected && !connecting && !hosting);
    }
    if (m_collaborationHostButton) {
        m_collaborationHostButton->setEnabled(!connected && !connecting && !hosting);
    }
    if (m_collaborationJoinButton) {
        m_collaborationJoinButton->setEnabled(!connected && !connecting && !hosting);
    }
    if (m_collaborationDisconnectButton) {
        m_collaborationDisconnectButton->setEnabled(connected || connecting || hosting);
    }
}

void MainWindow::refreshComponentsPanel(const QString& selectedTypeId) {
    if (!m_componentsList) {
        return;
    }

    QString effectiveSelectedTypeId = selectedTypeId;
    if (effectiveSelectedTypeId.isEmpty()) {
        if (const QListWidgetItem* current = m_componentsList->currentItem()) {
            effectiveSelectedTypeId = current->data(Qt::UserRole + 1).toString();
        }
    }

    QStringList warnings;
    const QVector<flowchart::InstalledComponentEntry> entries =
        flowchart::ComponentExtensionManager::loadInstalledComponentEntries(&warnings);

    QSignalBlocker blocker(m_componentsList);
    m_componentsList->clear();

    int selectedRow = -1;
    for (int index = 0; index < entries.size(); ++index) {
        const auto& entry = entries[index];
        auto* item = new QListWidgetItem(
            QStringLiteral("%1 (%2)").arg(entry.paletteLabel, entry.typeId),
            m_componentsList);
        item->setToolTip(entry.filePath);
        item->setData(Qt::UserRole, entry.filePath);
        item->setData(Qt::UserRole + 1, entry.typeId);
        item->setData(Qt::UserRole + 2, entry.definition);
        if (!effectiveSelectedTypeId.isEmpty() && entry.typeId == effectiveSelectedTypeId) {
            selectedRow = index;
        }
    }

    if (selectedRow < 0 && m_componentsList->count() > 0) {
        selectedRow = 0;
    }
    if (selectedRow >= 0) {
        m_componentsList->setCurrentRow(selectedRow);
    }

    updateManagedComponentDetails();
    if (!warnings.isEmpty()) {
        statusBar()->showMessage(
            QStringLiteral("组件加载存在警告：%1").arg(warnings.join(QStringLiteral(" | "))),
            6000);
    }
}

void MainWindow::updateManagedComponentDetails() {
    const QListWidgetItem* item = m_componentsList ? m_componentsList->currentItem() : nullptr;
    const bool hasSelection = item != nullptr;

    if (m_componentTypeValueLabel) {
        m_componentTypeValueLabel->setText(hasSelection ? item->data(Qt::UserRole + 1).toString() : QStringLiteral("-"));
    }

    if (!hasSelection) {
        if (m_componentLabelValueLabel) {
            m_componentLabelValueLabel->setText(QStringLiteral("-"));
        }
        if (m_componentFileValueLabel) {
            m_componentFileValueLabel->setText(QStringLiteral("-"));
        }
        if (m_componentJsonEdit) {
            m_componentJsonEdit->clear();
            m_componentJsonEdit->setEnabled(false);
        }
        if (m_componentSaveButton) {
            m_componentSaveButton->setEnabled(false);
        }
        if (m_componentExportButton) {
            m_componentExportButton->setEnabled(false);
        }
        if (m_componentDeleteButton) {
            m_componentDeleteButton->setEnabled(false);
        }
        return;
    }

    const QJsonObject definition = item->data(Qt::UserRole + 2).toJsonObject();
    if (m_componentLabelValueLabel) {
        m_componentLabelValueLabel->setText(
            definition.value(QStringLiteral("paletteLabel")).toString(item->data(Qt::UserRole + 1).toString()));
    }
    if (m_componentFileValueLabel) {
        m_componentFileValueLabel->setText(item->data(Qt::UserRole).toString());
    }
    if (m_componentJsonEdit) {
        m_componentJsonEdit->setEnabled(true);
        m_componentJsonEdit->setPlainText(QString::fromUtf8(QJsonDocument(definition).toJson(QJsonDocument::Indented)));
    }
    if (m_componentSaveButton) {
        m_componentSaveButton->setEnabled(true);
    }
    if (m_componentExportButton) {
        m_componentExportButton->setEnabled(true);
    }
    if (m_componentDeleteButton) {
        m_componentDeleteButton->setEnabled(true);
    }
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
        updateConnectorActions();
    });
    connect(m_session, &flowchart::EditorSession::documentChanged, this, [this]() {
        updateInspector();
        updateStatusSummary();
        updateConnectorActions();
    });
    connect(m_collaborationManager, &flowchart::PeerCollaborationManager::stateChanged, this, [this]() {
        updateCollaborationUi();
        updateStatusSummary();
    });
    connect(m_collaborationManager, &flowchart::PeerCollaborationManager::logMessage, this, [this](const QString& message) {
        if (m_collaborationLogEdit) {
            m_collaborationLogEdit->appendPlainText(message);
        }
        statusBar()->showMessage(message, 5000);
    });
    connect(m_collaborationManager, &flowchart::PeerCollaborationManager::remoteSnapshotApplied, this, [this]() {
        m_currentFile.clear();
        m_hasActiveDocument = true;
        updateDocumentAvailability();
        updateWindowTitle();
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
    connect(ui->connectorInsertButton, &QToolButton::clicked, this, [this]() { insertConnector(); });
    connect(ui->connectorAutoButton, &QToolButton::clicked, this, [this]() { m_session->resetSelectedConnectorBends(); });
    connect(ui->connectorVerticalButton, &QToolButton::clicked, this, [this]() { m_session->routeSelectedConnectorsVertical(); });
    connect(ui->connectorHorizontalButton, &QToolButton::clicked, this, [this]() { m_session->routeSelectedConnectorsHorizontal(); });
    connect(ui->connectorHeadCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (m_syncingConnectorControls || index < 0) {
            return;
        }
        static const QStringList styles = {
            QStringLiteral("classic"),
            QStringLiteral("stealth"),
            QStringLiteral("diamond"),
            QStringLiteral("circle"),
        };
        if (index < styles.size()) {
            m_session->setSelectedConnectorHeadStyle(styles[index]);
        }
    });
    connect(ui->connectorPatternCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (m_syncingConnectorControls || index < 0) {
            return;
        }
        static const QStringList styles = {
            QStringLiteral("solid"),
            QStringLiteral("dash"),
            QStringLiteral("dot"),
        };
        if (index < styles.size()) {
            m_session->setSelectedConnectorLineStyle(styles[index]);
        }
    });

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
    connect(m_importComponentsAction, &QAction::triggered, this, [this]() { importComponentPack(); });
    connect(m_createComponentAction, &QAction::triggered, this, [this]() { createCustomComponent(); });
    connect(m_openComponentsDirAction, &QAction::triggered, this, [this]() { openComponentsDirectory(); });
    connect(m_insertConnectorAction, &QAction::triggered, this, [this]() { insertConnector(); });
    connect(m_resetConnectorBendAction, &QAction::triggered, this, [this]() { m_session->resetSelectedConnectorBends(); });
    connect(m_routeConnectorVerticalAction, &QAction::triggered, this, [this]() { m_session->routeSelectedConnectorsVertical(); });
    connect(m_routeConnectorHorizontalAction, &QAction::triggered, this, [this]() { m_session->routeSelectedConnectorsHorizontal(); });
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

void MainWindow::importComponentPack() {
    const QString fileName = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("导入组件扩展包"),
        QString(),
        QStringLiteral("JSON (*.json)"));
    if (fileName.isEmpty()) {
        return;
    }

    QString error;
    QStringList importedTypeIds;
    if (!flowchart::ComponentExtensionManager::importComponentPack(fileName, &error, &importedTypeIds)) {
        QMessageBox::critical(this, QStringLiteral("导入失败"), error);
        return;
    }

    QStringList warnings;
    m_session->reloadShapeRegistry(&warnings);
    refreshPalette();
    refreshComponentsPanel(importedTypeIds.isEmpty() ? QString() : importedTypeIds.front());

    QString message = QStringLiteral("已导入 %1 个组件。").arg(importedTypeIds.size());
    if (!warnings.isEmpty()) {
        message += QStringLiteral("\n\n以下组件存在警告：\n%1").arg(warnings.join(QLatin1Char('\n')));
    }
    QMessageBox::information(this, QStringLiteral("导入完成"), message);
}

void MainWindow::createCustomComponent() {
    flowchart::CustomComponentDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString error;
    if (!flowchart::ComponentExtensionManager::saveComponentDefinition(dialog.componentDefinition(), &error)) {
        QMessageBox::critical(this, QStringLiteral("创建失败"), error);
        return;
    }

    QStringList warnings;
    m_session->reloadShapeRegistry(&warnings);
    refreshPalette();
    refreshComponentsPanel(dialog.componentDefinition().value(QStringLiteral("typeId")).toString());

    if (warnings.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("自定义组件已添加到图形栏。"), 4000);
        return;
    }
    QMessageBox::warning(
        this,
        QStringLiteral("组件已保存，但存在警告"),
        warnings.join(QLatin1Char('\n')));
}

void MainWindow::openComponentsDirectory() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(flowchart::ComponentExtensionManager::componentsDirectory()));
}

void MainWindow::insertConnector() {
    if (!m_hasActiveDocument) {
        return;
    }
    m_session->createConnector(m_view->sceneCenter());
}

void MainWindow::saveManagedComponentChanges() {
    QListWidgetItem* item = m_componentsList ? m_componentsList->currentItem() : nullptr;
    if (!item || !m_componentJsonEdit) {
        return;
    }

    QJsonParseError parseError;
    const QByteArray bytes = m_componentJsonEdit->toPlainText().trimmed().toUtf8();
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        QMessageBox::warning(this, QStringLiteral("JSON 无效"), parseError.errorString());
        return;
    }

    const QJsonObject definition = document.object();
    const QString originalFilePath = item->data(Qt::UserRole).toString();
    const QString originalTypeId = item->data(Qt::UserRole + 1).toString();
    const QString newTypeId = definition.value(QStringLiteral("typeId")).toString().trimmed();
    if (newTypeId.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("组件 typeId 不能为空。"));
        return;
    }

    const QString targetFilePath = flowchart::ComponentExtensionManager::componentFilePath(newTypeId);
    if (originalFilePath != targetFilePath && QFileInfo::exists(targetFilePath)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("目标组件 ID 已存在，请先修改为其他 ID。"));
        return;
    }

    QString error;
    if (!flowchart::ComponentExtensionManager::saveComponentDefinition(definition, &error)) {
        QMessageBox::critical(this, QStringLiteral("保存失败"), error);
        return;
    }

    if (originalTypeId != newTypeId) {
        flowchart::ComponentExtensionManager::deleteComponentFile(originalFilePath, nullptr);
    }

    QStringList warnings;
    m_session->reloadShapeRegistry(&warnings);
    refreshPalette();
    refreshComponentsPanel(newTypeId);
    if (warnings.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("组件已更新。"), 4000);
    } else {
        QMessageBox::warning(this, QStringLiteral("组件已保存，但存在警告"), warnings.join(QLatin1Char('\n')));
    }
}

void MainWindow::exportManagedComponent() {
    const QListWidgetItem* item = m_componentsList ? m_componentsList->currentItem() : nullptr;
    if (!item) {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("导出组件定义"),
        item->data(Qt::UserRole + 1).toString() + QStringLiteral(".json"),
        QStringLiteral("JSON (*.json)"));
    if (fileName.isEmpty()) {
        return;
    }
    if (QFileInfo(fileName).suffix().isEmpty()) {
        fileName += QStringLiteral(".json");
    }

    QJsonObject definition = item->data(Qt::UserRole + 2).toJsonObject();
    if (m_componentJsonEdit && !m_componentJsonEdit->toPlainText().trimmed().isEmpty()) {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(m_componentJsonEdit->toPlainText().trimmed().toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && document.isObject()) {
            definition = document.object();
        }
    }

    QString error;
    if (!flowchart::ComponentExtensionManager::saveComponentDocument(
            definition,
            fileName,
            &error)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"), error);
        return;
    }
    statusBar()->showMessage(QStringLiteral("组件已导出。"), 4000);
}

void MainWindow::deleteManagedComponent() {
    QListWidgetItem* item = m_componentsList ? m_componentsList->currentItem() : nullptr;
    if (!item) {
        return;
    }

    const QString typeId = item->data(Qt::UserRole + 1).toString();
    const auto result = QMessageBox::warning(
        this,
        QStringLiteral("删除组件"),
        QStringLiteral("确定删除组件 %1 吗？此操作不可撤销。").arg(typeId),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (result != QMessageBox::Yes) {
        return;
    }

    QString error;
    if (!flowchart::ComponentExtensionManager::deleteComponentFile(item->data(Qt::UserRole).toString(), &error)) {
        QMessageBox::critical(this, QStringLiteral("删除失败"), error);
        return;
    }

    QStringList warnings;
    m_session->reloadShapeRegistry(&warnings);
    refreshPalette();
    refreshComponentsPanel();
    if (warnings.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("组件已删除。"), 4000);
    } else {
        QMessageBox::warning(this, QStringLiteral("组件已删除，但存在警告"), warnings.join(QLatin1Char('\n')));
    }
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

void MainWindow::startHostingCollaboration() {
    if (!m_collaborationManager || !m_collaborationPortSpin) {
        return;
    }
    m_collaborationManager->setLocalDisplayName(
        m_collaborationNameEdit ? m_collaborationNameEdit->text() : QString());

    QString error;
    if (!m_collaborationManager->startHosting(static_cast<quint16>(m_collaborationPortSpin->value()), &error)) {
        QMessageBox::critical(this, QStringLiteral("开房失败"), error);
        return;
    }
    updateCollaborationUi();
}

void MainWindow::joinPeerCollaboration() {
    if (!m_collaborationManager || !m_collaborationPortSpin || !m_collaborationHostEdit) {
        return;
    }
    if (!maybeSave()) {
        return;
    }

    m_collaborationManager->setLocalDisplayName(
        m_collaborationNameEdit ? m_collaborationNameEdit->text() : QString());

    QString error;
    if (!m_collaborationManager->joinPeer(
            m_collaborationHostEdit->text(),
            static_cast<quint16>(m_collaborationPortSpin->value()),
            &error)) {
        QMessageBox::critical(this, QStringLiteral("加入房间失败"), error);
        return;
    }
    updateCollaborationUi();
}

void MainWindow::disconnectCollaboration() {
    if (!m_collaborationManager) {
        return;
    }
    m_collaborationManager->disconnectSession();
    updateCollaborationUi();
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

    if (m_collaborationManager) {
        m_collaborationManager->disconnectSession();
    }
    discardRecoverySnapshot();
    event->accept();
}

MainWindow::~MainWindow() {
    delete ui;
}
