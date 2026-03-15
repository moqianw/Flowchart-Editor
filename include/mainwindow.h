#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "canvasview.h"
#include "editorsession.h"

#include <QCloseEvent>
#include <QMainWindow>
#include <QColorDialog>
#include <QDockWidget>
#include <QFileDialog>
#include <QListWidget>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
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
    void setupWorkspaceEnhancements();
    void setupWelcomeView();
    void setupInspectorDock();
    void setupAdvancedActions();
    void setupPersistence();
    void updateDocumentAvailability();
    void updateInspector();
    void updateStatusSummary();
    void applyInspectorGeometry();
    void updateRecentFilesMenu();
    void updateRecentFilesWelcomeList();
    void startNewDocument();
    void openDocumentDialog();
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
    QMenu* m_recentFilesMenu = nullptr;
    QLabel* m_statusSummaryLabel = nullptr;
    QDockWidget* m_inspectorDock = nullptr;
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
    QTimer* m_autoSaveTimer = nullptr;
    bool m_syncingInspector = false;
    bool m_hasActiveDocument = false;
};
#endif // MAINWINDOW_H
