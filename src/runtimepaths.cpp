#include "runtimepaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace flowchart {

namespace {

QString ensureDirectory(const QString& path) {
    QDir().mkpath(path);
    return QDir(path).absolutePath();
}

}  // namespace

QString RuntimePaths::executableDirectory() {
    QString directory = QCoreApplication::applicationDirPath();
    if (directory.isEmpty()) {
        directory = QDir::currentPath();
    }
    return QDir(directory).absolutePath();
}

QString RuntimePaths::runtimeDataDirectory() {
    return ensureDirectory(QDir(executableDirectory()).filePath(QStringLiteral("runtime-data")));
}

QString RuntimePaths::settingsFilePath() {
    return QDir(runtimeDataDirectory()).filePath(QStringLiteral("flowchart-editor.ini"));
}

QString RuntimePaths::recoveryFilePath() {
    return QDir(runtimeDataDirectory()).filePath(QStringLiteral("recovery.json"));
}

QString RuntimePaths::componentsDirectory() {
    return ensureDirectory(QDir(runtimeDataDirectory()).filePath(QStringLiteral("components")));
}

}  // namespace flowchart
