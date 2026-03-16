#include "shaperegistry.h"

#include "componentextensionmanager.h"

#include <QJsonArray>
#include <QPainter>
#include <QPixmap>
#include <QSet>
#include <QtMath>

#include <algorithm>
#include <utility>

namespace flowchart {

namespace {

using JsonField = std::pair<QString, QJsonValue>;

QJsonObject jsonObject(std::initializer_list<JsonField> fields) {
    QJsonObject object;
    for (const auto& field : fields) {
        object.insert(field.first, field.second);
    }
    return object;
}

QJsonObject pointSpec(qreal x, qreal y) {
    return jsonObject({
        {QStringLiteral("x"), x},
        {QStringLiteral("y"), y},
    });
}

QJsonArray pointsToJson(const QVector<QPointF>& points) {
    QJsonArray array;
    for (const QPointF& point : points) {
        array.push_back(pointSpec(point.x(), point.y()));
    }
    return array;
}

QJsonObject command(const QString& op, std::initializer_list<JsonField> fields = {}) {
    QJsonObject object;
    object.insert(QStringLiteral("op"), op);
    for (const auto& field : fields) {
        object.insert(field.first, field.second);
    }
    return object;
}

QJsonObject drawSpec(std::initializer_list<QJsonObject> commands) {
    QJsonArray array;
    for (const QJsonObject& item : commands) {
        array.push_back(item);
    }
    return jsonObject({{QStringLiteral("commands"), array}});
}

QJsonObject presetPorts(const QString& preset) {
    return jsonObject({{QStringLiteral("kind"), preset}});
}

QJsonObject customPorts(const QVector<QPointF>& points) {
    return jsonObject({
        {QStringLiteral("kind"), QStringLiteral("custom")},
        {QStringLiteral("points"), pointsToJson(points)},
    });
}

QColor colorFromJson(const QJsonObject& object, const QColor& fallback) {
    if (!object.contains(QStringLiteral("r"))) {
        return fallback;
    }
    return QColor(
        object.value(QStringLiteral("r")).toInt(fallback.red()),
        object.value(QStringLiteral("g")).toInt(fallback.green()),
        object.value(QStringLiteral("b")).toInt(fallback.blue()),
        object.value(QStringLiteral("a")).toInt(fallback.alpha()));
}

QSizeF sizeFromJson(const QJsonObject& object, const QSizeF& fallback) {
    const qreal width = object.value(QStringLiteral("w")).toDouble(fallback.width());
    const qreal height = object.value(QStringLiteral("h")).toDouble(fallback.height());
    if (width <= 0.0 || height <= 0.0) {
        return fallback;
    }
    return QSizeF(width, height);
}

bool usesNormalizedCoordinates(const QJsonObject& object, bool fallback = true) {
    return object.value(QStringLiteral("normalized")).toBool(fallback);
}

qreal resolveLength(qreal value, qreal reference, bool normalized) {
    return normalized ? value * reference : value;
}

QPointF resolvePoint(const QRectF& rect, const QJsonObject& object, bool normalizedFallback = true) {
    const bool normalized = usesNormalizedCoordinates(object, normalizedFallback);
    const qreal x = object.value(QStringLiteral("x")).toDouble();
    const qreal y = object.value(QStringLiteral("y")).toDouble();
    return QPointF(
        rect.left() + resolveLength(x, rect.width(), normalized),
        rect.top() + resolveLength(y, rect.height(), normalized));
}

QRectF resolveSubRect(const QRectF& rect, const QJsonObject& object, bool normalizedFallback = true) {
    const bool normalized = usesNormalizedCoordinates(object, normalizedFallback);
    const qreal x = object.value(QStringLiteral("x")).toDouble();
    const qreal y = object.value(QStringLiteral("y")).toDouble();
    const qreal width = object.value(QStringLiteral("w")).toDouble(1.0);
    const qreal height = object.value(QStringLiteral("h")).toDouble(1.0);
    return QRectF(
        rect.left() + resolveLength(x, rect.width(), normalized),
        rect.top() + resolveLength(y, rect.height(), normalized),
        resolveLength(width, rect.width(), normalized),
        resolveLength(height, rect.height(), normalized));
}

qreal resolveRadius(qreal value, const QRectF& rect) {
    const qreal reference = std::min(rect.width(), rect.height());
    if (qAbs(value) <= 1.0) {
        return value * reference;
    }
    return value;
}

QVector<QPointF> fourWayPorts(const QRectF& rect) {
    return {
        QPointF(rect.left(), rect.center().y()),
        QPointF(rect.center().x(), rect.top()),
        QPointF(rect.center().x(), rect.bottom()),
        QPointF(rect.right(), rect.center().y()),
    };
}

QVector<QPointF> normalizedPortsToScenePoints(
    const QRectF& rect,
    const QJsonArray& points,
    bool normalizedFallback = true) {
    QVector<QPointF> ports;
    ports.reserve(points.size());
    for (const QJsonValue& value : points) {
        if (!value.isObject()) {
            continue;
        }
        ports.push_back(resolvePoint(rect, value.toObject(), normalizedFallback));
    }
    return ports;
}

QPainterPath buildPathFromSpec(const QJsonObject& spec, const QRectF& rect) {
    const QJsonArray commands = spec.value(QStringLiteral("commands")).toArray();
    QPainterPath path;
    if (commands.isEmpty()) {
        path.addRect(rect);
        return path;
    }

    for (const QJsonValue& value : commands) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject commandObject = value.toObject();
        const QString op = commandObject.value(QStringLiteral("op")).toString();
        if (op == QStringLiteral("moveTo")) {
            path.moveTo(resolvePoint(rect, commandObject));
        } else if (op == QStringLiteral("lineTo")) {
            path.lineTo(resolvePoint(rect, commandObject));
        } else if (op == QStringLiteral("quadTo")) {
            const QJsonObject control = jsonObject({
                {QStringLiteral("x"), commandObject.value(QStringLiteral("cx"))},
                {QStringLiteral("y"), commandObject.value(QStringLiteral("cy"))},
                {QStringLiteral("normalized"), commandObject.value(QStringLiteral("normalized"))},
            });
            path.quadTo(resolvePoint(rect, control), resolvePoint(rect, commandObject));
        } else if (op == QStringLiteral("cubicTo")) {
            const QJsonObject c1 = jsonObject({
                {QStringLiteral("x"), commandObject.value(QStringLiteral("c1x"))},
                {QStringLiteral("y"), commandObject.value(QStringLiteral("c1y"))},
                {QStringLiteral("normalized"), commandObject.value(QStringLiteral("normalized"))},
            });
            const QJsonObject c2 = jsonObject({
                {QStringLiteral("x"), commandObject.value(QStringLiteral("c2x"))},
                {QStringLiteral("y"), commandObject.value(QStringLiteral("c2y"))},
                {QStringLiteral("normalized"), commandObject.value(QStringLiteral("normalized"))},
            });
            path.cubicTo(resolvePoint(rect, c1), resolvePoint(rect, c2), resolvePoint(rect, commandObject));
        } else if (op == QStringLiteral("rect")) {
            path.addRect(resolveSubRect(rect, commandObject));
        } else if (op == QStringLiteral("ellipse")) {
            path.addEllipse(resolveSubRect(rect, commandObject));
        } else if (op == QStringLiteral("roundedRect")) {
            const QRectF subRect = resolveSubRect(rect, commandObject);
            const qreal rx = resolveRadius(commandObject.value(QStringLiteral("rx")).toDouble(0.12), subRect);
            const qreal ry = resolveRadius(commandObject.value(QStringLiteral("ry")).toDouble(
                commandObject.value(QStringLiteral("rx")).toDouble(0.12)), subRect);
            path.addRoundedRect(subRect, rx, ry);
        } else if (op == QStringLiteral("polygon")) {
            const QJsonArray points = commandObject.value(QStringLiteral("points")).toArray();
            if (points.isEmpty()) {
                continue;
            }

            QVector<QPointF> polygonPoints = normalizedPortsToScenePoints(
                rect,
                points,
                usesNormalizedCoordinates(commandObject));
            if (polygonPoints.isEmpty()) {
                continue;
            }

            path.moveTo(polygonPoints.front());
            for (int index = 1; index < polygonPoints.size(); ++index) {
                path.lineTo(polygonPoints[index]);
            }
            if (commandObject.value(QStringLiteral("closed")).toBool(true)) {
                path.closeSubpath();
            }
        } else if (op == QStringLiteral("arcMoveTo")) {
            path.arcMoveTo(
                resolveSubRect(rect, commandObject),
                commandObject.value(QStringLiteral("start")).toDouble());
        } else if (op == QStringLiteral("arcTo")) {
            path.arcTo(
                resolveSubRect(rect, commandObject),
                commandObject.value(QStringLiteral("start")).toDouble(),
                commandObject.value(QStringLiteral("sweep")).toDouble());
        } else if (op == QStringLiteral("close")) {
            path.closeSubpath();
        } else if (op == QStringLiteral("parallelogram")) {
            const QRectF subRect = resolveSubRect(rect, commandObject);
            qreal offset = 0.0;
            if (commandObject.contains(QStringLiteral("offset"))) {
                const qreal rawOffset = commandObject.value(QStringLiteral("offset")).toDouble(0.18);
                offset = qAbs(rawOffset) <= 1.0 ? rawOffset * subRect.width() : rawOffset;
            } else {
                const qreal angle = commandObject.value(QStringLiteral("angle")).toDouble(80.0);
                const qreal radians = qDegreesToRadians(angle);
                offset = qFuzzyIsNull(qTan(radians)) ? 0.0 : (subRect.height() / qTan(radians));
            }
            offset = std::clamp(offset, 0.0, subRect.width() * 0.45);

            path.moveTo(subRect.left() + offset, subRect.top());
            path.lineTo(subRect.right(), subRect.top());
            path.lineTo(subRect.right() - offset, subRect.bottom());
            path.lineTo(subRect.left(), subRect.bottom());
            path.closeSubpath();
        }
    }

    if (path.isEmpty()) {
        path.addRect(rect);
    }
    return path;
}

QJsonObject legacyShapeToDrawSpec(const QJsonObject& shape) {
    const QString shapeKind = shape.value(QStringLiteral("kind")).toString().trimmed();
    if (shapeKind == QStringLiteral("ellipse")) {
        return drawSpec({command(QStringLiteral("ellipse"))});
    }
    if (shapeKind == QStringLiteral("roundedRect")) {
        return drawSpec({
            command(QStringLiteral("roundedRect"), {
                {QStringLiteral("rx"), shape.value(QStringLiteral("rx")).toDouble(12.0)},
                {QStringLiteral("ry"), shape.value(QStringLiteral("ry")).toDouble(shape.value(QStringLiteral("rx")).toDouble(12.0))},
            }),
        });
    }
    if (shapeKind == QStringLiteral("diamond")) {
        return drawSpec({
            command(QStringLiteral("polygon"), {
                {QStringLiteral("points"), pointsToJson({
                    QPointF(0.5, 0.0),
                    QPointF(0.0, 0.5),
                    QPointF(0.5, 1.0),
                    QPointF(1.0, 0.5),
                })},
            }),
        });
    }
    if (shapeKind == QStringLiteral("parallelogram")) {
        return drawSpec({
            command(QStringLiteral("parallelogram"), {
                {QStringLiteral("angle"), shape.value(QStringLiteral("angle")).toDouble(80.0)},
            }),
        });
    }
    if (shapeKind == QStringLiteral("hexagon")) {
        return drawSpec({
            command(QStringLiteral("polygon"), {
                {QStringLiteral("points"), pointsToJson({
                    QPointF(0.25, 0.0),
                    QPointF(0.75, 0.0),
                    QPointF(1.0, 0.5),
                    QPointF(0.75, 1.0),
                    QPointF(0.25, 1.0),
                    QPointF(0.0, 0.5),
                })},
            }),
        });
    }
    if (shapeKind == QStringLiteral("terminator")) {
        return drawSpec({
            command(QStringLiteral("roundedRect"), {
                {QStringLiteral("rx"), 0.5},
                {QStringLiteral("ry"), 0.5},
            }),
        });
    }
    if (shapeKind == QStringLiteral("document")) {
        const qreal ratio = shape.value(QStringLiteral("ratio")).toDouble(0.15);
        return drawSpec({
            command(QStringLiteral("moveTo"), {{QStringLiteral("x"), 0.0}, {QStringLiteral("y"), 1.0 - ratio}}),
            command(QStringLiteral("lineTo"), {{QStringLiteral("x"), 0.0}, {QStringLiteral("y"), 0.0}}),
            command(QStringLiteral("lineTo"), {{QStringLiteral("x"), 1.0}, {QStringLiteral("y"), 0.0}}),
            command(QStringLiteral("lineTo"), {{QStringLiteral("x"), 1.0}, {QStringLiteral("y"), 1.0 - ratio}}),
            command(QStringLiteral("arcTo"), {
                {QStringLiteral("x"), 0.0},
                {QStringLiteral("y"), 1.0 - ratio * 2.0},
                {QStringLiteral("w"), 0.5},
                {QStringLiteral("h"), ratio * 2.0},
                {QStringLiteral("start"), 180.0},
                {QStringLiteral("sweep"), 180.0},
            }),
            command(QStringLiteral("arcTo"), {
                {QStringLiteral("x"), 0.5},
                {QStringLiteral("y"), 1.0 - ratio * 2.0},
                {QStringLiteral("w"), 0.5},
                {QStringLiteral("h"), ratio * 2.0},
                {QStringLiteral("start"), 0.0},
                {QStringLiteral("sweep"), 180.0},
            }),
        });
    }
    if (shapeKind == QStringLiteral("polygon")) {
        return drawSpec({
            command(QStringLiteral("polygon"), {
                {QStringLiteral("points"), shape.value(QStringLiteral("points")).toArray()},
            }),
        });
    }

    return drawSpec({command(QStringLiteral("rect"))});
}

QIcon buildPaletteIcon(const ShapeDefinition& definition) {
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    ItemStyle style;
    if (definition.initializeStyle) {
        definition.initializeStyle(style);
    }

    const QRectF previewRect(10.0, 10.0, 44.0, 44.0);
    const QPainterPath path = buildShapePath(definition, previewRect);

    QPen pen(style.strokeColor);
    pen.setWidth(std::max(style.strokeWidth, 2));
    painter.setPen(pen);
    painter.setBrush(style.fillColor);
    painter.drawPath(path);

    return QIcon(pixmap);
}

ShapeDefinition makeDefinition(
    const QString& typeId,
    const QString& paletteLabel,
    const QString& iconPath,
    const QSizeF& defaultSize,
    const QJsonObject& defaultProps,
    const QJsonObject& drawSpecValue,
    const QJsonObject& portsSpecValue,
    const std::function<void(ItemStyle&)>& initializeStyle = {}) {
    ShapeDefinition definition;
    definition.typeId = typeId;
    definition.paletteLabel = paletteLabel;
    definition.iconPath = iconPath;
    definition.defaultSize = defaultSize;
    definition.defaultProps = defaultProps;
    definition.drawSpec = drawSpecValue;
    definition.portsSpec = portsSpecValue;
    definition.initializeStyle = initializeStyle;
    definition.paletteIcon = buildPaletteIcon(definition);
    return definition;
}

bool buildCustomDefinition(const QJsonObject& component, ShapeDefinition* outDefinition, QString* errorMessage) {
    if (!outDefinition) {
        return false;
    }

    const QString typeId = component.value(QStringLiteral("typeId")).toString().trimmed();
    if (typeId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Missing component typeId.");
        }
        return false;
    }

    if (component.value(QStringLiteral("connector")).toBool(false)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Connectors are not component definitions: %1.").arg(typeId);
        }
        return false;
    }

    ShapeDefinition definition;
    definition.typeId = typeId;
    definition.paletteLabel = component.value(QStringLiteral("paletteLabel")).toString(typeId);
    definition.iconPath = component.value(QStringLiteral("iconPath")).toString();
    definition.defaultSize = sizeFromJson(
        component.value(QStringLiteral("defaultSize")).toObject(),
        QSizeF(140.0, 90.0));
    definition.custom = true;
    definition.defaultProps = component.value(QStringLiteral("defaultProps")).toObject();

    QJsonObject drawSpecValue = component.value(QStringLiteral("drawSpec")).toObject();
    const QJsonObject legacyShape = component.value(QStringLiteral("shape")).toObject();
    if (drawSpecValue.isEmpty() && legacyShape.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Component %1 is missing drawSpec.").arg(typeId);
        }
        return false;
    }
    if (drawSpecValue.isEmpty()) {
        drawSpecValue = legacyShapeToDrawSpec(legacyShape);
    }
    if (drawSpecValue.value(QStringLiteral("commands")).toArray().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Component %1 has no valid drawSpec.commands.").arg(typeId);
        }
        return false;
    }
    definition.drawSpec = drawSpecValue;

    const QJsonObject portsSpecValue = component.value(QStringLiteral("ports")).toObject();
    definition.portsSpec = portsSpecValue.isEmpty() ? presetPorts(QStringLiteral("fourWay")) : portsSpecValue;

    const QJsonObject style = component.value(QStringLiteral("style")).toObject();
    definition.initializeStyle = [style](ItemStyle& targetStyle) {
        if (style.contains(QStringLiteral("strokeColor")) && style.value(QStringLiteral("strokeColor")).isObject()) {
            targetStyle.strokeColor = colorFromJson(style.value(QStringLiteral("strokeColor")).toObject(), targetStyle.strokeColor);
        }
        if (style.contains(QStringLiteral("fillColor")) && style.value(QStringLiteral("fillColor")).isObject()) {
            targetStyle.fillColor = colorFromJson(style.value(QStringLiteral("fillColor")).toObject(), targetStyle.fillColor);
        }
        if (style.contains(QStringLiteral("strokeWidth"))) {
            targetStyle.strokeWidth = style.value(QStringLiteral("strokeWidth")).toInt(targetStyle.strokeWidth);
        }
    };
    definition.paletteIcon = buildPaletteIcon(definition);

    *outDefinition = definition;
    return true;
}

}  // namespace

QPainterPath buildShapePath(const ShapeDefinition& definition, const QRectF& rect, const QJsonObject&) {
    return buildPathFromSpec(definition.drawSpec, rect);
}

QVector<QPointF> buildShapePorts(const ShapeDefinition& definition, const QRectF& rect, const QJsonObject&) {
    const QJsonObject spec = definition.portsSpec;
    if (spec.contains(QStringLiteral("points")) && spec.value(QStringLiteral("points")).isArray()) {
        return normalizedPortsToScenePoints(rect, spec.value(QStringLiteral("points")).toArray(), usesNormalizedCoordinates(spec));
    }

    QString kind = spec.value(QStringLiteral("kind")).toString(QStringLiteral("fourWay"));
    if (kind == QStringLiteral("preset")) {
        kind = spec.value(QStringLiteral("preset")).toString(QStringLiteral("fourWay"));
    }

    if (kind == QStringLiteral("none")) {
        return {};
    }
    if (kind == QStringLiteral("twoVertical")) {
        return {
            QPointF(rect.center().x(), rect.top()),
            QPointF(rect.center().x(), rect.bottom()),
        };
    }
    if (kind == QStringLiteral("twoHorizontal")) {
        return {
            QPointF(rect.left(), rect.center().y()),
            QPointF(rect.right(), rect.center().y()),
        };
    }
    return fourWayPorts(rect);
}

ShapeRegistry::ShapeRegistry() {
    reload();
}

const ShapeDefinition* ShapeRegistry::definition(const QString& typeId) const {
    const auto it = m_indexByType.find(typeId);
    if (it == m_indexByType.end()) {
        return nullptr;
    }
    return &m_definitions[*it];
}

QVector<const ShapeDefinition*> ShapeRegistry::paletteDefinitions() const {
    QVector<const ShapeDefinition*> result;
    result.reserve(m_definitions.size());
    for (const ShapeDefinition& definition : m_definitions) {
        result.push_back(&definition);
    }
    return result;
}

bool ShapeRegistry::reload(QStringList* warnings) {
    m_definitions.clear();
    m_indexByType.clear();
    registerDefaults();

    QStringList localWarnings;
    const QVector<QJsonObject> components = ComponentExtensionManager::loadInstalledComponents(&localWarnings);
    for (const QJsonObject& component : components) {
        ShapeDefinition definition;
        QString error;
        if (!buildCustomDefinition(component, &definition, &error)) {
            localWarnings.push_back(error);
            continue;
        }

        if (!addDefinition(definition, &error)) {
            localWarnings.push_back(error);
        }
    }

    if (warnings) {
        *warnings = localWarnings;
    }
    return localWarnings.isEmpty();
}

bool ShapeRegistry::addDefinition(const ShapeDefinition& definition, QString* errorMessage) {
    if (definition.typeId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Component typeId is empty.");
        }
        return false;
    }
    if (m_indexByType.contains(definition.typeId)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Component typeId '%1' already exists.").arg(definition.typeId);
        }
        return false;
    }

    m_indexByType.insert(definition.typeId, m_definitions.size());
    m_definitions.push_back(definition);
    return true;
}

void ShapeRegistry::registerDefaults() {
    const QVector<ShapeDefinition> defaults = {
        makeDefinition(
            QStringLiteral("Ellipse"),
            QStringLiteral("椭圆"),
            QStringLiteral(":/image/Elliesp.png"),
            QSizeF(100.0, 100.0),
            {},
            drawSpec({command(QStringLiteral("ellipse"))}),
            presetPorts(QStringLiteral("fourWay"))),
        makeDefinition(
            QStringLiteral("Rectangle"),
            QStringLiteral("矩形"),
            QStringLiteral(":/image/Rectangle.png"),
            QSizeF(100.0, 100.0),
            {},
            drawSpec({command(QStringLiteral("rect"))}),
            presetPorts(QStringLiteral("fourWay"))),
        makeDefinition(
            QStringLiteral("Diamond"),
            QStringLiteral("菱形"),
            QStringLiteral(":/image/Diamond.png"),
            QSizeF(100.0, 100.0),
            {},
            drawSpec({
                command(QStringLiteral("polygon"), {
                    {QStringLiteral("points"), pointsToJson({
                        QPointF(0.5, 0.0),
                        QPointF(0.0, 0.5),
                        QPointF(0.5, 1.0),
                        QPointF(1.0, 0.5),
                    })},
                }),
            }),
            presetPorts(QStringLiteral("fourWay"))),
        makeDefinition(
            QStringLiteral("RoundedRectangle"),
            QStringLiteral("圆角矩形"),
            QStringLiteral(":/image/RoundedRectangle.png"),
            QSizeF(100.0, 100.0),
            {},
            drawSpec({
                command(QStringLiteral("roundedRect"), {
                    {QStringLiteral("rx"), 0.1},
                    {QStringLiteral("ry"), 0.1},
                }),
            }),
            presetPorts(QStringLiteral("fourWay"))),
        makeDefinition(
            QStringLiteral("Parallelogram"),
            QStringLiteral("平行四边形"),
            QStringLiteral(":/image/Parallelogram.png"),
            QSizeF(100.0, 100.0),
            {},
            drawSpec({
                command(QStringLiteral("parallelogram"), {
                    {QStringLiteral("angle"), 80.0},
                }),
            }),
            presetPorts(QStringLiteral("fourWay"))),
        makeDefinition(
            QStringLiteral("Start_or_Terminator"),
            QStringLiteral("开始/结束"),
            QStringLiteral(":/image/Start_or_Terminator.png"),
            QSizeF(300.0, 50.0),
            {},
            drawSpec({
                command(QStringLiteral("roundedRect"), {
                    {QStringLiteral("rx"), 0.5},
                    {QStringLiteral("ry"), 0.5},
                }),
            }),
            presetPorts(QStringLiteral("twoVertical"))),
        makeDefinition(
            QStringLiteral("Subprocess"),
            QStringLiteral("子流程"),
            QStringLiteral(":/image/Subprocess.png"),
            QSizeF(200.0, 100.0),
            {},
            drawSpec({
                command(QStringLiteral("rect")),
                command(QStringLiteral("moveTo"), {{QStringLiteral("x"), 0.15}, {QStringLiteral("y"), 0.0}}),
                command(QStringLiteral("lineTo"), {{QStringLiteral("x"), 0.15}, {QStringLiteral("y"), 1.0}}),
                command(QStringLiteral("moveTo"), {{QStringLiteral("x"), 0.85}, {QStringLiteral("y"), 0.0}}),
                command(QStringLiteral("lineTo"), {{QStringLiteral("x"), 0.85}, {QStringLiteral("y"), 1.0}}),
            }),
            presetPorts(QStringLiteral("fourWay"))),
        makeDefinition(
            QStringLiteral("Database"),
            QStringLiteral("数据库"),
            QStringLiteral(":/image/Database.png"),
            QSizeF(200.0, 100.0),
            {},
            drawSpec({
                command(QStringLiteral("ellipse"), {
                    {QStringLiteral("x"), 0.0},
                    {QStringLiteral("y"), 0.0},
                    {QStringLiteral("w"), 1.0},
                    {QStringLiteral("h"), 0.3},
                }),
                command(QStringLiteral("moveTo"), {{QStringLiteral("x"), 0.0}, {QStringLiteral("y"), 0.15}}),
                command(QStringLiteral("lineTo"), {{QStringLiteral("x"), 0.0}, {QStringLiteral("y"), 0.85}}),
                command(QStringLiteral("moveTo"), {{QStringLiteral("x"), 1.0}, {QStringLiteral("y"), 0.15}}),
                command(QStringLiteral("lineTo"), {{QStringLiteral("x"), 1.0}, {QStringLiteral("y"), 0.85}}),
                command(QStringLiteral("arcMoveTo"), {
                    {QStringLiteral("x"), 0.0},
                    {QStringLiteral("y"), 0.7},
                    {QStringLiteral("w"), 1.0},
                    {QStringLiteral("h"), 0.3},
                    {QStringLiteral("start"), 180.0},
                }),
                command(QStringLiteral("arcTo"), {
                    {QStringLiteral("x"), 0.0},
                    {QStringLiteral("y"), 0.7},
                    {QStringLiteral("w"), 1.0},
                    {QStringLiteral("h"), 0.3},
                    {QStringLiteral("start"), 180.0},
                    {QStringLiteral("sweep"), 180.0},
                }),
            }),
            presetPorts(QStringLiteral("fourWay"))),
        makeDefinition(
            QStringLiteral("Document"),
            QStringLiteral("文档"),
            QStringLiteral(":/image/Document.png"),
            QSizeF(200.0, 100.0),
            {},
            legacyShapeToDrawSpec(jsonObject({
                {QStringLiteral("kind"), QStringLiteral("document")},
                {QStringLiteral("ratio"), 0.15},
            })),
            presetPorts(QStringLiteral("fourWay"))),
        makeDefinition(
            QStringLiteral("DataStorage"),
            QStringLiteral("数据存储"),
            QStringLiteral(":/image/DataStorage.png"),
            QSizeF(200.0, 100.0),
            {},
            drawSpec({
                command(QStringLiteral("arcMoveTo"), {
                    {QStringLiteral("x"), 0.0},
                    {QStringLiteral("y"), 0.0},
                    {QStringLiteral("w"), 0.15},
                    {QStringLiteral("h"), 1.0},
                    {QStringLiteral("start"), 90.0},
                }),
                command(QStringLiteral("arcTo"), {
                    {QStringLiteral("x"), 0.0},
                    {QStringLiteral("y"), 0.0},
                    {QStringLiteral("w"), 0.15},
                    {QStringLiteral("h"), 1.0},
                    {QStringLiteral("start"), 90.0},
                    {QStringLiteral("sweep"), 180.0},
                }),
                command(QStringLiteral("lineTo"), {{QStringLiteral("x"), 1.0}, {QStringLiteral("y"), 1.0}}),
                command(QStringLiteral("arcTo"), {
                    {QStringLiteral("x"), 0.85},
                    {QStringLiteral("y"), 0.0},
                    {QStringLiteral("w"), 0.15},
                    {QStringLiteral("h"), 1.0},
                    {QStringLiteral("start"), 90.0},
                    {QStringLiteral("sweep"), -180.0},
                }),
                command(QStringLiteral("lineTo"), {{QStringLiteral("x"), 0.075}, {QStringLiteral("y"), 0.0}}),
                command(QStringLiteral("close")),
            }),
            presetPorts(QStringLiteral("fourWay"))),
        makeDefinition(
            QStringLiteral("Textpointer"),
            QStringLiteral("文本"),
            QStringLiteral(":/image/Textpointer.png"),
            QSizeF(200.0, 100.0),
            {},
            drawSpec({command(QStringLiteral("rect"))}),
            presetPorts(QStringLiteral("fourWay")),
            [](ItemStyle& style) {
                style.strokeColor = QColor(0, 0, 0, 0);
                style.fillColor = QColor(255, 255, 255, 0);
            }),
    };

    for (const ShapeDefinition& definition : defaults) {
        addDefinition(definition, nullptr);
    }
}

}  // namespace flowchart
