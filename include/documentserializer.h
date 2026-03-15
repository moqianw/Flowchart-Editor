#pragma once

#include "diagramtypes.h"

namespace flowchart {

class DocumentSerializer {
public:
    static bool save(
        const std::vector<std::unique_ptr<DiagramItemModel>>& items,
        const QString& fileName,
        QString* errorMessage);

    static std::vector<std::unique_ptr<DiagramItemModel>> load(
        const QString& fileName,
        QString* errorMessage);
};

}  // namespace flowchart
