#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "canvasview.h"
#include "editorsession.h"
#include "peercollaborationmanager.h"

#include <QCloseEvent>
#include <QMainWindow>
#include <QColorDialog>
#include <QDockWidget>
#include <QFileDialog>
#include <QListWidget>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTimer>
#include <QAction>
#include <QVBoxLayout>

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

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    bool maybeSave();
    bool saveToFile(const QString& fileName);
    bool openFromFile(const QString& fileName, bool rememberRecent = true);
    void updateWindowTitle();
    void bindUi();
    void populatePalette();
    void refreshPalette();
    void setupWorkspaceEnhancements();
    void setupWelcomeView();
    void setupInspectorDock();
    void setupComponentsDock();
    void setupCollaborationDock();
    void setupAdvancedActions();
    void setupPersistence();
    void updateDocumentAvailability();
    void updateInspector();
    void updateStatusSummary();
    void updateConnectorActions();
    void updateCollaborationUi();
    void applyInspectorGeometry();
    void updateRecentFilesMenu();
    void updateRecentFilesWelcomeList();
    void refreshComponentsPanel(const QString& selectedTypeId = QString());
    void updateManagedComponentDetails();
    void startNewDocument();
    void openDocumentDialog();
    void importComponentPack();
    void createCustomComponent();
    void openComponentsDirectory();
    void insertConnector();
    void saveManagedComponentChanges();
    void exportManagedComponent();
    void deleteManagedComponent();
    void startHostingCollaboration();
    void joinPeerCollaboration();
    void disconnectCollaboration();
    void persistRecoverySnapshot();
    void discardRecoverySnapshot();
    void restoreRecoverySnapshotIfAvailable();
    QString currentSingleSelectedItemId() const;

    Ui::MainWindow* ui;
    QString m_currentFile;
    flowchart::EditorSession* m_session;
    flowchart::CanvasView* m_view;
    QStackedWidget* m_workspaceStack = nullptr;
    QWidget* m_welcomeView = nullptr;
    QListWidget* m_recentFilesList = nullptr;
    QLabel* m_recentFilesEmptyLabel = nullptr;
    QList<QPushButton*> m_paletteButtons;
    QVBoxLayout* m_paletteLayout = nullptr;
    QAction* m_autoLayoutAction = nullptr;
    QAction* m_validateAction = nullptr;
    QAction* m_snapToGridAction = nullptr;
    QAction* m_exportMermaidAction = nullptr;
    QAction* m_importComponentsAction = nullptr;
    QAction* m_createComponentAction = nullptr;
    QAction* m_openComponentsDirAction = nullptr;
    QAction* m_insertConnectorAction = nullptr;
    QAction* m_resetConnectorBendAction = nullptr;
    QAction* m_routeConnectorVerticalAction = nullptr;
    QAction* m_routeConnectorHorizontalAction = nullptr;
    QMenu* m_recentFilesMenu = nullptr;
    QMenu* m_connectorMenu = nullptr;
    QLabel* m_statusSummaryLabel = nullptr;
    QDockWidget* m_inspectorDock = nullptr;
    QDockWidget* m_componentsDock = nullptr;
    QDockWidget* m_collaborationDock = nullptr;
    QLabel* m_itemTypeLabel = nullptr;
    QLabel* m_itemIdLabel = nullptr;
    QLineEdit* m_textEdit = nullptr;
    QDoubleSpinBox* m_xSpin = nullptr;
    QDoubleSpinBox* m_ySpin = nullptr;
    QDoubleSpinBox* m_widthSpin = nullptr;
    QDoubleSpinBox* m_heightSpin = nullptr;
    QDoubleSpinBox* m_rotationSpin = nullptr;
    QDoubleSpinBox* m_scaleSpin = nullptr;
    QSpinBox* m_fillAlphaSpin = nullptr;
    QSpinBox* m_strokeWidthSpin = nullptr;
    QListWidget* m_componentsList = nullptr;
    QLabel* m_componentTypeValueLabel = nullptr;
    QLabel* m_componentLabelValueLabel = nullptr;
    QLabel* m_componentFileValueLabel = nullptr;
    QPlainTextEdit* m_componentJsonEdit = nullptr;
    QPushButton* m_componentSaveButton = nullptr;
    QPushButton* m_componentExportButton = nullptr;
    QPushButton* m_componentDeleteButton = nullptr;
    QLineEdit* m_collaborationNameEdit = nullptr;
    QLineEdit* m_collaborationHostEdit = nullptr;
    QSpinBox* m_collaborationPortSpin = nullptr;
    QLabel* m_collaborationStatusLabel = nullptr;
    QLabel* m_collaborationPeerLabel = nullptr;
    QPushButton* m_collaborationHostButton = nullptr;
    QPushButton* m_collaborationJoinButton = nullptr;
    QPushButton* m_collaborationDisconnectButton = nullptr;
    QPlainTextEdit* m_collaborationLogEdit = nullptr;
    QTimer* m_autoSaveTimer = nullptr;
    flowchart::PeerCollaborationManager* m_collaborationManager = nullptr;
    bool m_syncingInspector = false;
    bool m_syncingConnectorControls = false;
    bool m_hasActiveDocument = false;
};
#endif // MAINWINDOW_H
