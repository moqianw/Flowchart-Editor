#pragma once

#include "diagramtypes.h"

#include <QHash>
#include <QPainterPath>
#include <QSizeF>
#include <QVector>

#include <functional>

namespace flowchart {

struct ShapeDefinition {
    QString typeId;
    QString paletteLabel;
    QString iconPath;
    QSizeF defaultSize;
    bool connector = false;
    QJsonObject defaultProps;
    std::function<void(ItemStyle&)> initializeStyle;
    std::function<QPainterPath(const QRectF&, const QJsonObject&)> buildPath;
    std::function<QVector<QPointF>(const QRectF&, const QJsonObject&)> buildPorts;
};

class ShapeRegistry {
public:
    ShapeRegistry();

    const ShapeDefinition* definition(const QString& typeId) const;
    QVector<const ShapeDefinition*> paletteDefinitions() const;

private:
    void registerDefaults();

    QVector<ShapeDefinition> m_definitions;
    QHash<QString, int> m_indexByType;
};

}  // namespace flowchart
