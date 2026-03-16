#include "componentextensionmanager.h"
#include "runtimepaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>

namespace flowchart {

namespace {

QString normalizeComponentFileBase(QString typeId) {
    typeId = typeId.trimmed();
    QString normalized;
    normalized.reserve(typeId.size());
    for (const QChar ch : typeId) {
        if (ch.isLetterOrNumber() || ch == QLatin1Char('_') || ch == QLatin1Char('-')) {
            normalized.append(ch);
        } else {
            normalized.append(QLatin1Char('_'));
        }
    }
    if (normalized.isEmpty()) {
        normalized = QStringLiteral("custom_component");
    }
    return normalized;
}

QVector<QJsonObject> extractDefinitions(const QJsonObject& root, QString* errorMessage) {
    QVector<QJsonObject> definitions;

    if (root.contains(QStringLiteral("components")) && root.value(QStringLiteral("components")).isArray()) {
        const QJsonArray components = root.value(QStringLiteral("components")).toArray();
        definitions.reserve(components.size());
        for (const QJsonValue& value : components) {
            if (!value.isObject()) {
                continue;
            }
            definitions.push_back(value.toObject());
        }
    } else if (root.contains(QStringLiteral("component")) && root.value(QStringLiteral("component")).isObject()) {
        definitions.push_back(root.value(QStringLiteral("component")).toObject());
    } else if (root.contains(QStringLiteral("typeId"))) {
        definitions.push_back(root);
    }

    if (definitions.isEmpty() && errorMessage) {
        *errorMessage = QStringLiteral("No component definitions found.");
    }
    return definitions;
}

bool validateDefinition(const QJsonObject& definition, QString* errorMessage) {
    const QString typeId = definition.value(QStringLiteral("typeId")).toString().trimmed();
    if (typeId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Component typeId is required.");
        }
        return false;
    }

    const QJsonObject drawSpec = definition.value(QStringLiteral("drawSpec")).toObject();
    const QJsonObject legacyShape = definition.value(QStringLiteral("shape")).toObject();
    if (drawSpec.value(QStringLiteral("commands")).toArray().isEmpty()
        && legacyShape.value(QStringLiteral("kind")).toString().trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Component drawSpec.commands is required.");
        }
        return false;
    }

    return true;
}

QJsonObject withVersion(const QJsonObject& definition) {
    QJsonObject stored = definition;
    stored.insert(QStringLiteral("version"), 1);
    return stored;
}

bool readDefinitionFile(
    const QString& filePath,
    QVector<QJsonObject>* definitions,
    QString* errorMessage) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = parseError.errorString();
        }
        return false;
    }

    QString localError;
    const QVector<QJsonObject> extracted = extractDefinitions(document.object(), &localError);
    if (!localError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = localError;
        }
        return false;
    }

    if (definitions) {
        *definitions = extracted;
    }
    return true;
}

}  // namespace

QString ComponentExtensionManager::componentsDirectory() {
    return RuntimePaths::componentsDirectory();
}

QString ComponentExtensionManager::componentFilePath(const QString& typeId) {
    return QDir(componentsDirectory()).filePath(normalizeComponentFileBase(typeId) + QStringLiteral(".json"));
}

QVector<QJsonObject> ComponentExtensionManager::loadInstalledComponents(QStringList* warnings) {
    QVector<QJsonObject> definitions;
    QDir directory(componentsDirectory());
    const QFileInfoList files = directory.entryInfoList(QStringList{QStringLiteral("*.json")}, QDir::Files, QDir::Name);
    for (const QFileInfo& fileInfo : files) {
        QString error;
        QVector<QJsonObject> extracted;
        if (!readDefinitionFile(fileInfo.absoluteFilePath(), &extracted, &error)) {
            if (warnings) {
                warnings->push_back(QStringLiteral("%1: %2").arg(fileInfo.fileName(), error));
            }
            continue;
        }
        for (const QJsonObject& definition : extracted) {
            definitions.push_back(definition);
        }
    }
    return definitions;
}

QVector<InstalledComponentEntry> ComponentExtensionManager::loadInstalledComponentEntries(QStringList* warnings) {
    QVector<InstalledComponentEntry> entries;
    QDir directory(componentsDirectory());
    const QFileInfoList files = directory.entryInfoList(QStringList{QStringLiteral("*.json")}, QDir::Files, QDir::Name);
    for (const QFileInfo& fileInfo : files) {
        QString error;
        QVector<QJsonObject> extracted;
        if (!readDefinitionFile(fileInfo.absoluteFilePath(), &extracted, &error)) {
            if (warnings) {
                warnings->push_back(QStringLiteral("%1: %2").arg(fileInfo.fileName(), error));
            }
            continue;
        }

        for (const QJsonObject& definition : extracted) {
            InstalledComponentEntry entry;
            entry.typeId = definition.value(QStringLiteral("typeId")).toString().trimmed();
            entry.paletteLabel = definition.value(QStringLiteral("paletteLabel")).toString(entry.typeId);
            entry.filePath = fileInfo.absoluteFilePath();
            entry.definition = definition;
            entries.push_back(entry);
        }
    }
    return entries;
}

bool ComponentExtensionManager::importComponentPack(
    const QString& fileName,
    QString* errorMessage,
    QStringList* importedTypeIds) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = parseError.errorString();
        }
        return false;
    }

    QString localError;
    const QVector<QJsonObject> definitions = extractDefinitions(document.object(), &localError);
    if (!localError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = localError;
        }
        return false;
    }

    QStringList importedIds;
    for (const QJsonObject& definition : definitions) {
        if (!saveComponentDefinition(definition, &localError)) {
            if (errorMessage) {
                *errorMessage = localError;
            }
            return false;
        }
        importedIds.push_back(definition.value(QStringLiteral("typeId")).toString().trimmed());
    }

    if (importedTypeIds) {
        *importedTypeIds = importedIds;
    }
    return true;
}

bool ComponentExtensionManager::saveComponentDocument(
    const QJsonObject& definition,
    const QString& fileName,
    QString* errorMessage) {
    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    const QJsonDocument document(withVersion(definition));
    if (file.write(document.toJson(QJsonDocument::Indented)) == -1) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        file.cancelWriting();
        return false;
    }

    if (!file.commit()) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    return true;
}

bool ComponentExtensionManager::saveComponentDefinition(
    const QJsonObject& definition,
    QString* errorMessage) {
    QString localError;
    if (!validateDefinition(definition, &localError)) {
        if (errorMessage) {
            *errorMessage = localError;
        }
        return false;
    }

    const QString typeId = definition.value(QStringLiteral("typeId")).toString().trimmed();
    return saveComponentDocument(definition, componentFilePath(typeId), errorMessage);
}

bool ComponentExtensionManager::deleteComponentFile(
    const QString& filePath,
    QString* errorMessage) {
    if (filePath.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Component file path is empty.");
        }
        return false;
    }
    if (!QFileInfo::exists(filePath)) {
        return true;
    }
    if (QFile::remove(filePath)) {
        return true;
    }
    if (errorMessage) {
        *errorMessage = QStringLiteral("Failed to delete component file.");
    }
    return false;
}

}  // namespace flowchart
