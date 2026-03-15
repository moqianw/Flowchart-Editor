#include "mainwindow.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("FlowchartEditor"));
    QApplication::setOrganizationDomain(QStringLiteral("local.flowchart.editor"));
    QApplication::setApplicationName(QStringLiteral("FlowchartEditor"));

    MainWindow w;
    w.show();
    return a.exec();
}
