#pragma once

#include "diagramtypes.h"

#include <QDateTime>
#include <QStringList>

namespace flowchart {

struct RecoverySnapshot {
    QString sourceFile;
    QDateTime savedAt;
    std::vector<std::unique_ptr<DiagramItemModel>> items;
    bool loaded = false;

    bool isValid() const;
};

class SessionPersistence {
public:
    static QString recoveryFilePath();
    static bool hasRecoverySnapshot();
    static bool saveRecoverySnapshot(
        const std::vector<std::unique_ptr<DiagramItemModel>>& items,
        const QString& sourceFile,
        QString* errorMessage);
    static RecoverySnapshot loadRecoverySnapshot(QString* errorMessage);
    static void discardRecoverySnapshot();

    static QStringList recentFiles();
    static void rememberRecentFile(const QString& fileName, int maxFiles = 8);
    static void removeRecentFile(const QString& fileName);
};

}  // namespace flowchart
