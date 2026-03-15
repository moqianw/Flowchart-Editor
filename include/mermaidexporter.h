#pragma once

#include "diagramdocument.h"

#include <QString>

namespace flowchart {

class MermaidExporter {
public:
    static bool save(const DiagramDocument& document, const QString& fileName, QString* errorMessage);
};

}  // namespace flowchart
