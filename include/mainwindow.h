#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "canvasview.h"
#include "editorsession.h"

#include <QMainWindow>
#include <QColorDialog>
#include <QDockWidget>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
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

private:
    bool maybeSave();
    bool saveToFile(const QString& fileName);
    void updateWindowTitle();
    void bindUi();
    void populatePalette();
    void setupWorkspaceEnhancements();
    void setupInspectorDock();
    void setupAdvancedActions();
    void updateInspector();
    void updateStatusSummary();
    void applyInspectorGeometry();
    QString currentSingleSelectedItemId() const;

    Ui::MainWindow* ui;
    QString m_currentFile;
    flowchart::EditorSession* m_session;
    flowchart::CanvasView* m_view;
    QVBoxLayout* m_paletteLayout = nullptr;
    QAction* m_autoLayoutAction = nullptr;
    QAction* m_validateAction = nullptr;
    QAction* m_snapToGridAction = nullptr;
    QAction* m_exportMermaidAction = nullptr;
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
    bool m_syncingInspector = false;
};
#endif // MAINWINDOW_H
