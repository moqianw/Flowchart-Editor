#include "sessionpersistence.h"

#include "documentserializer.h"
#include "runtimepaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSettings>

namespace flowchart {

namespace {

const char kRecentFilesKey[] = "recentFiles";

QSettings settings() {
    return QSettings(RuntimePaths::settingsFilePath(), QSettings::IniFormat);
}

QString normalizedFileName(const QString& fileName) {
    return QDir::toNativeSeparators(QFileInfo(fileName).absoluteFilePath());
}

}  // namespace

bool RecoverySnapshot::isValid() const {
    return loaded;
}

QString SessionPersistence::recoveryFilePath() {
    return RuntimePaths::recoveryFilePath();
}

bool SessionPersistence::hasRecoverySnapshot() {
    return QFileInfo::exists(recoveryFilePath());
}

bool SessionPersistence::saveRecoverySnapshot(
    const std::vector<std::unique_ptr<DiagramItemModel>>& items,
    const QString& sourceFile,
    QString* errorMessage) {
    QSaveFile file(recoveryFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    const QJsonObject bundle{
        {QStringLiteral("version"), 1},
        {QStringLiteral("sourceFile"), sourceFile},
        {QStringLiteral("savedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {QStringLiteral("document"), DocumentSerializer::toJsonObject(items)},
    };

    if (file.write(QJsonDocument(bundle).toJson(QJsonDocument::Indented)) == -1) {
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

RecoverySnapshot SessionPersistence::loadRecoverySnapshot(QString* errorMessage) {
    RecoverySnapshot snapshot;

    QFile file(recoveryFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return snapshot;
    }

    QJsonParseError parseError;
    const QJsonDocument bundle = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !bundle.isObject()) {
        if (errorMessage) {
            *errorMessage = parseError.errorString();
        }
        return snapshot;
    }

    const QJsonObject root = bundle.object();
    snapshot.loaded = root.contains(QStringLiteral("document")) && root.value(QStringLiteral("document")).isObject();
    snapshot.sourceFile = root.value(QStringLiteral("sourceFile")).toString();
    snapshot.savedAt = QDateTime::fromString(root.value(QStringLiteral("savedAt")).toString(), Qt::ISODate);
    snapshot.items = DocumentSerializer::fromJsonObject(root.value(QStringLiteral("document")).toObject(), errorMessage);
    return snapshot;
}

void SessionPersistence::discardRecoverySnapshot() {
    QFile::remove(recoveryFilePath());
}

QStringList SessionPersistence::recentFiles() {
    QStringList files = settings().value(QLatin1StringView(kRecentFilesKey)).toStringList();
    QStringList normalized;
    for (const QString& fileName : files) {
        if (fileName.isEmpty()) {
            continue;
        }
        const QString absolute = normalizedFileName(fileName);
        if (!QFileInfo::exists(absolute) || normalized.contains(absolute)) {
            continue;
        }
        normalized.push_back(absolute);
    }
    return normalized;
}

void SessionPersistence::rememberRecentFile(const QString& fileName, int maxFiles) {
    if (fileName.isEmpty()) {
        return;
    }

    QStringList files = recentFiles();
    const QString normalized = normalizedFileName(fileName);
    files.removeAll(normalized);
    files.push_front(normalized);
    while (files.size() > maxFiles) {
        files.removeLast();
    }

    settings().setValue(QLatin1StringView(kRecentFilesKey), files);
}

void SessionPersistence::removeRecentFile(const QString& fileName) {
    if (fileName.isEmpty()) {
        return;
    }

    QStringList files = recentFiles();
    files.removeAll(normalizedFileName(fileName));
    settings().setValue(QLatin1StringView(kRecentFilesKey), files);
}

}  // namespace flowchart
