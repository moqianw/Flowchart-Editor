#pragma once

#include "diagramtypes.h"

#include <QHash>
#include <QIcon>
#include <QPainterPath>
#include <QSizeF>
#include <QVector>

#include <functional>

namespace flowchart {

struct ShapeDefinition {
    QString typeId;
    QString paletteLabel;
    QString iconPath;
    QIcon paletteIcon;
    QSizeF defaultSize;
    bool custom = false;
    QJsonObject defaultProps;
    QJsonObject drawSpec;
    QJsonObject portsSpec;
    std::function<void(ItemStyle&)> initializeStyle;
};

QPainterPath buildShapePath(const ShapeDefinition& definition, const QRectF& rect, const QJsonObject& props = {});
QVector<QPointF> buildShapePorts(const ShapeDefinition& definition, const QRectF& rect, const QJsonObject& props = {});

class ShapeRegistry {
public:
    ShapeRegistry();

    const ShapeDefinition* definition(const QString& typeId) const;
    QVector<const ShapeDefinition*> paletteDefinitions() const;
    bool reload(QStringList* warnings = nullptr);

private:
    void registerDefaults();
    bool addDefinition(const ShapeDefinition& definition, QString* errorMessage);

    QVector<ShapeDefinition> m_definitions;
    QHash<QString, int> m_indexByType;
};

}  // namespace flowchart
