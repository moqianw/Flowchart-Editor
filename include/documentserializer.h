#pragma once

#include "diagramtypes.h"

#include <QJsonObject>

namespace flowchart {

class DocumentSerializer {
public:
    static QJsonObject toJsonObject(const std::vector<std::unique_ptr<DiagramItemModel>>& items);
    static std::vector<std::unique_ptr<DiagramItemModel>> fromJsonObject(
        const QJsonObject& root,
        QString* errorMessage);

    static bool save(
        const std::vector<std::unique_ptr<DiagramItemModel>>& items,
        const QString& fileName,
        QString* errorMessage);

    static std::vector<std::unique_ptr<DiagramItemModel>> load(
        const QString& fileName,
        QString* errorMessage);
};

}  // namespace flowchart
