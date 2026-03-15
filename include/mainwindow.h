#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "canvasview.h"
#include "editorsession.h"

#include <QMainWindow>
#include <QColorDialog>
#include <QFileDialog>
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

    Ui::MainWindow* ui;
    QString m_currentFile;
    flowchart::EditorSession* m_session;
    flowchart::CanvasView* m_view;
    QVBoxLayout* m_paletteLayout = nullptr;
};
#endif // MAINWINDOW_H
