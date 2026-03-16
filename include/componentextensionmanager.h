#pragma once

#include <QJsonObject>
#include <QStringList>
#include <QVector>

namespace flowchart {

struct InstalledComponentEntry {
    QString typeId;
    QString paletteLabel;
    QString filePath;
    QJsonObject definition;
};

class ComponentExtensionManager {
public:
    static QString componentsDirectory();
    static QString componentFilePath(const QString& typeId);
    static QVector<QJsonObject> loadInstalledComponents(QStringList* warnings = nullptr);
    static QVector<InstalledComponentEntry> loadInstalledComponentEntries(QStringList* warnings = nullptr);
    static bool importComponentPack(
        const QString& fileName,
        QString* errorMessage,
        QStringList* importedTypeIds = nullptr);
    static bool saveComponentDocument(
        const QJsonObject& definition,
        const QString& fileName,
        QString* errorMessage);
    static bool saveComponentDefinition(
        const QJsonObject& definition,
        QString* errorMessage);
    static bool deleteComponentFile(
        const QString& filePath,
        QString* errorMessage);
};

}  // namespace flowchart
