#include "shaperegistry.h"

#include <QtMath>

namespace flowchart {

namespace {

QVector<QPointF> fourWayPorts(const QRectF& rect) {
    return {
        QPointF(rect.left(), rect.center().y()),
        QPointF(rect.center().x(), rect.top()),
        QPointF(rect.center().x(), rect.bottom()),
        QPointF(rect.right(), rect.center().y()),
    };
}

ShapeDefinition makeDefinition(
    const QString& typeId,
    const QString& paletteLabel,
    const QString& iconPath,
    const QSizeF& defaultSize,
    const QJsonObject& defaultProps,
    const std::function<void(ItemStyle&)>& initializeStyle,
    const std::function<QPainterPath(const QRectF&, const QJsonObject&)>& buildPath,
    const std::function<QVector<QPointF>(const QRectF&, const QJsonObject&)>& buildPorts,
    bool connector = false) {
    ShapeDefinition definition;
    definition.typeId = typeId;
    definition.paletteLabel = paletteLabel;
    definition.iconPath = iconPath;
    definition.defaultSize = defaultSize;
    definition.defaultProps = defaultProps;
    definition.initializeStyle = initializeStyle;
    definition.buildPath = buildPath;
    definition.buildPorts = buildPorts;
    definition.connector = connector;
    return definition;
}

}  // namespace

ShapeRegistry::ShapeRegistry() {
    registerDefaults();
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

void ShapeRegistry::registerDefaults() {
    m_definitions = {
        makeDefinition(
            QStringLiteral("Ellipse"),
            QStringLiteral("椭圆"),
            QStringLiteral(":/image/Elliesp.png"),
            QSizeF(100.0, 100.0),
            {},
            {},
            [](const QRectF& rect, const QJsonObject&) {
                QPainterPath path;
                path.addEllipse(rect);
                return path;
            },
            [](const QRectF& rect, const QJsonObject&) { return fourWayPorts(rect); }),
        makeDefinition(
            QStringLiteral("Rectangle"),
            QStringLiteral("矩形"),
            QStringLiteral(":/image/Rectangle.png"),
            QSizeF(100.0, 100.0),
            {},
            {},
            [](const QRectF& rect, const QJsonObject&) {
                QPainterPath path;
                path.addRect(rect);
                return path;
            },
            [](const QRectF& rect, const QJsonObject&) { return fourWayPorts(rect); }),
        makeDefinition(
            QStringLiteral("Diamond"),
            QStringLiteral("菱形"),
            QStringLiteral(":/image/Diamond.png"),
            QSizeF(100.0, 100.0),
            {},
            {},
            [](const QRectF& rect, const QJsonObject&) {
                QPainterPath path;
                path.moveTo(rect.center().x(), rect.top());
                path.lineTo(rect.left(), rect.center().y());
                path.lineTo(rect.center().x(), rect.bottom());
                path.lineTo(rect.right(), rect.center().y());
                path.closeSubpath();
                return path;
            },
            [](const QRectF& rect, const QJsonObject&) { return fourWayPorts(rect); }),
        makeDefinition(
            QStringLiteral("RoundedRectangle"),
            QStringLiteral("圆角矩形"),
            QStringLiteral(":/image/RoundedRectangle.png"),
            QSizeF(100.0, 100.0),
            QJsonObject{{QStringLiteral("rx"), 10}, {QStringLiteral("ry"), 10}},
            {},
            [](const QRectF& rect, const QJsonObject& props) {
                const qreal rx = props.value(QStringLiteral("rx")).toDouble(10.0);
                const qreal ry = props.value(QStringLiteral("ry")).toDouble(10.0);
                QPainterPath path;
                path.addRoundedRect(rect, rx, ry);
                return path;
            },
            [](const QRectF& rect, const QJsonObject&) { return fourWayPorts(rect); }),
        makeDefinition(
            QStringLiteral("Parallelogram"),
            QStringLiteral("平行四边形"),
            QStringLiteral(":/image/Parallelogram.png"),
            QSizeF(100.0, 100.0),
            QJsonObject{{QStringLiteral("angle"), 80.0}},
            {},
            [](const QRectF& rect, const QJsonObject& props) {
                const qreal angle = props.value(QStringLiteral("angle")).toDouble(80.0);
                const qreal offset = rect.height() / qTan(qDegreesToRadians(angle));
                const QPointF p1(rect.left() + offset, rect.top());
                const QPointF p2(rect.right(), rect.top());
                const QPointF p3(rect.right() - offset, rect.bottom());
                const QPointF p4(rect.left(), rect.bottom());
                QPainterPath path;
                path.moveTo(p1);
                path.lineTo(p2);
                path.lineTo(p3);
                path.lineTo(p4);
                path.closeSubpath();
                return path;
            },
            [](const QRectF& rect, const QJsonObject& props) {
                const qreal angle = props.value(QStringLiteral("angle")).toDouble(80.0);
                const qreal offset = rect.height() / qTan(qDegreesToRadians(angle));
                const QPointF p1(rect.left() + offset, rect.top());
                const QPointF p2(rect.right(), rect.top());
                const QPointF p3(rect.right() - offset, rect.bottom());
                const QPointF p4(rect.left(), rect.bottom());
                return QVector<QPointF>{
                    (p1 + p2) / 2.0,
                    (p2 + p3) / 2.0,
                    (p3 + p4) / 2.0,
                    (p4 + p1) / 2.0,
                };
            }),
        makeDefinition(
            QStringLiteral("Start_or_Terminator"),
            QStringLiteral("开始/结束"),
            QStringLiteral(":/image/Start_or_Terminator.png"),
            QSizeF(300.0, 50.0),
            {},
            {},
            [](const QRectF& rect, const QJsonObject&) {
                const qreal radius = rect.height() / 2.0;
                QPainterPath path;
                path.arcMoveTo(QRectF(rect.left(), rect.top(), rect.height(), rect.height()), 90);
                path.arcTo(QRectF(rect.left(), rect.top(), rect.height(), rect.height()), 90, 180);
                path.lineTo(rect.right() - radius, rect.bottom());
                path.arcTo(QRectF(rect.right() - rect.height(), rect.top(), rect.height(), rect.height()), -90, 180);
                path.lineTo(rect.left() + radius, rect.top());
                path.closeSubpath();
                return path;
            },
            [](const QRectF& rect, const QJsonObject&) {
                return QVector<QPointF>{
                    QPointF(rect.center().x(), rect.top()),
                    QPointF(rect.center().x(), rect.bottom()),
                };
            }),
        makeDefinition(
            QStringLiteral("Subprocess"),
            QStringLiteral("子流程"),
            QStringLiteral(":/image/Subprocess.png"),
            QSizeF(200.0, 100.0),
            QJsonObject{{QStringLiteral("offset"), 30}},
            {},
            [](const QRectF& rect, const QJsonObject& props) {
                const qreal offset = props.value(QStringLiteral("offset")).toDouble(30.0);
                QPainterPath path;
                path.addRect(rect);
                path.moveTo(rect.left() + offset, rect.top());
                path.lineTo(rect.left() + offset, rect.bottom());
                path.moveTo(rect.right() - offset, rect.top());
                path.lineTo(rect.right() - offset, rect.bottom());
                return path;
            },
            [](const QRectF& rect, const QJsonObject&) { return fourWayPorts(rect); }),
        makeDefinition(
            QStringLiteral("Database"),
            QStringLiteral("数据库"),
            QStringLiteral(":/image/Database.png"),
            QSizeF(200.0, 100.0),
            QJsonObject{{QStringLiteral("ratio"), 0.3}},
            {},
            [](const QRectF& rect, const QJsonObject& props) {
                const qreal ratio = props.value(QStringLiteral("ratio")).toDouble(0.3);
                const qreal ellipseHeight = rect.height() * ratio;
                QPainterPath path;
                path.addEllipse(QRectF(rect.left(), rect.top(), rect.width(), ellipseHeight));
                path.moveTo(rect.left(), rect.top() + ellipseHeight / 2.0);
                path.lineTo(rect.left(), rect.bottom() - ellipseHeight / 2.0);
                path.moveTo(rect.right(), rect.top() + ellipseHeight / 2.0);
                path.lineTo(rect.right(), rect.bottom() - ellipseHeight / 2.0);
                path.arcMoveTo(QRectF(rect.left(), rect.bottom() - ellipseHeight, rect.width(), ellipseHeight), 180);
                path.arcTo(
                    QRectF(rect.left(), rect.bottom() - ellipseHeight, rect.width(), ellipseHeight),
                    180,
                    180);
                return path;
            },
            [](const QRectF& rect, const QJsonObject&) { return fourWayPorts(rect); }),
        makeDefinition(
            QStringLiteral("Document"),
            QStringLiteral("文档"),
            QStringLiteral(":/image/Document.png"),
            QSizeF(200.0, 100.0),
            QJsonObject{{QStringLiteral("ratio"), 0.15}},
            {},
            [](const QRectF& rect, const QJsonObject& props) {
                const qreal ratio = props.value(QStringLiteral("ratio")).toDouble(0.15);
                const qreal waveHeight = rect.height() * ratio;
                QPainterPath path;
                path.moveTo(rect.left(), rect.bottom() - waveHeight);
                path.lineTo(rect.left(), rect.top());
                path.lineTo(rect.right(), rect.top());
                path.lineTo(rect.right(), rect.bottom() - waveHeight);
                path.arcTo(
                    QRectF(rect.left(), rect.bottom() - 2.0 * waveHeight, rect.width() / 2.0, 2.0 * waveHeight),
                    180,
                    180);
                path.arcTo(
                    QRectF(
                        rect.left() + rect.width() / 2.0,
                        rect.bottom() - 2.0 * waveHeight,
                        rect.width() / 2.0,
                        2.0 * waveHeight),
                    0,
                    180);
                return path;
            },
            [](const QRectF& rect, const QJsonObject&) { return fourWayPorts(rect); }),
        makeDefinition(
            QStringLiteral("DataStorage"),
            QStringLiteral("数据存储"),
            QStringLiteral(":/image/DataStorage.png"),
            QSizeF(200.0, 100.0),
            QJsonObject{{QStringLiteral("offset"), 15}},
            {},
            [](const QRectF& rect, const QJsonObject& props) {
                const qreal offset = props.value(QStringLiteral("offset")).toDouble(15.0);
                QPainterPath path;
                path.arcMoveTo(QRectF(rect.left(), rect.top(), offset * 2.0, rect.height()), 90);
                path.arcTo(QRectF(rect.left(), rect.top(), offset * 2.0, rect.height()), 90, 180);
                path.lineTo(rect.right(), rect.bottom());
                path.arcTo(
                    QRectF(rect.right() - 2.0 * offset, rect.top(), offset * 2.0, rect.height()),
                    90,
                    -180);
                path.lineTo(rect.left() + offset, rect.top());
                path.closeSubpath();
                return path;
            },
            [](const QRectF& rect, const QJsonObject&) { return fourWayPorts(rect); }),
        makeDefinition(
            QStringLiteral("Textpointer"),
            QStringLiteral("文本"),
            QStringLiteral(":/image/Textpointer.png"),
            QSizeF(200.0, 100.0),
            {},
            [](ItemStyle& style) {
                style.strokeColor = QColor(0, 0, 0, 0);
                style.fillColor = QColor(255, 255, 255, 0);
            },
            [](const QRectF& rect, const QJsonObject&) {
                QPainterPath path;
                path.addRect(rect);
                return path;
            },
            [](const QRectF& rect, const QJsonObject&) { return fourWayPorts(rect); }),
        makeDefinition(
            QStringLiteral("AllowLine"),
            QStringLiteral("箭头连线"),
            QStringLiteral(":/image/Allowline.png"),
            QSizeF(100.0, 100.0),
            {},
            [](ItemStyle& style) {
                style.fillColor = QColor(0, 0, 0, 255);
            },
            {},
            {},
            true),
    };

    for (int index = 0; index < m_definitions.size(); ++index) {
        m_indexByType.insert(m_definitions[index].typeId, index);
    }
}

}  // namespace flowchart
