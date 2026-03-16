#include "mainwindow.h"
#include "runtimepaths.h"

#include <QApplication>
#include <QSettings>

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("FlowchartEditor"));
    QApplication::setOrganizationDomain(QStringLiteral("local.flowchart.editor"));
    QApplication::setApplicationName(QStringLiteral("FlowchartEditor"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, flowchart::RuntimePaths::runtimeDataDirectory());

    MainWindow w;
    w.show();
    return a.exec();
}
