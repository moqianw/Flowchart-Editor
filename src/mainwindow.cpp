#include "mainwindow.h"

#include "ui_mainwindow.h"

#include "palettebutton.h"

#include <QFileInfo>
#include <QFrame>
#include <QMessageBox>
#include <QScrollArea>
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
    bindUi();
    updateWindowTitle();
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

void MainWindow::bindUi() {
    connect(m_session, &flowchart::EditorSession::dirtyChanged, this, [this](bool) {
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
}

MainWindow::~MainWindow() {
    delete ui;
}
