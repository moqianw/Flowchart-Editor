#include "documentserializer.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

namespace flowchart {

namespace {

QJsonObject colorToJson(const QColor& color) {
    return QJsonObject{
        {QStringLiteral("r"), color.red()},
        {QStringLiteral("g"), color.green()},
        {QStringLiteral("b"), color.blue()},
        {QStringLiteral("a"), color.alpha()},
    };
}

QColor colorFromJson(const QJsonValue& value) {
    const QJsonObject object = value.toObject();
    return QColor(
        object.value(QStringLiteral("r")).toInt(),
        object.value(QStringLiteral("g")).toInt(),
        object.value(QStringLiteral("b")).toInt(),
        object.value(QStringLiteral("a")).toInt(255));
}

QJsonObject pointToJson(const QPointF& point) {
    return QJsonObject{{QStringLiteral("x"), point.x()}, {QStringLiteral("y"), point.y()}};
}

QPointF pointFromJson(const QJsonValue& value) {
    const QJsonObject object = value.toObject();
    return QPointF(object.value(QStringLiteral("x")).toDouble(), object.value(QStringLiteral("y")).toDouble());
}

QJsonObject rectToJson(const QRectF& rect) {
    return QJsonObject{
        {QStringLiteral("x"), rect.x()},
        {QStringLiteral("y"), rect.y()},
        {QStringLiteral("w"), rect.width()},
        {QStringLiteral("h"), rect.height()},
    };
}

QRectF rectFromJson(const QJsonValue& value) {
    const QJsonObject object = value.toObject();
    return QRectF(
        object.value(QStringLiteral("x")).toDouble(),
        object.value(QStringLiteral("y")).toDouble(),
        object.value(QStringLiteral("w")).toDouble(),
        object.value(QStringLiteral("h")).toDouble());
}

QJsonObject endpointToJson(const EndpointRef& endpoint) {
    return QJsonObject{
        {QStringLiteral("itemId"), endpoint.itemId},
        {QStringLiteral("portIndex"), endpoint.portIndex},
        {QStringLiteral("position"), pointToJson(endpoint.position)},
    };
}

EndpointRef endpointFromJson(const QJsonValue& value) {
    EndpointRef endpoint;
    const QJsonObject object = value.toObject();
    endpoint.itemId = object.value(QStringLiteral("itemId")).toString();
    endpoint.portIndex = object.value(QStringLiteral("portIndex")).toInt(-1);
    endpoint.position = pointFromJson(object.value(QStringLiteral("position")));
    return endpoint;
}

QJsonObject styleToJson(const ItemStyle& style) {
    return QJsonObject{
        {QStringLiteral("strokeColor"), colorToJson(style.strokeColor)},
        {QStringLiteral("fillColor"), colorToJson(style.fillColor)},
        {QStringLiteral("strokeWidth"), style.strokeWidth},
        {QStringLiteral("font"), style.font.toString()},
        {QStringLiteral("bold"), style.bold},
        {QStringLiteral("italic"), style.italic},
        {QStringLiteral("underline"), style.underline},
        {QStringLiteral("rotation"), style.rotation},
        {QStringLiteral("scale"), style.scale},
    };
}

ItemStyle styleFromJson(const QJsonValue& value) {
    ItemStyle style;
    const QJsonObject object = value.toObject();
    style.strokeColor = colorFromJson(object.value(QStringLiteral("strokeColor")));
    style.fillColor = colorFromJson(object.value(QStringLiteral("fillColor")));
    style.strokeWidth = object.value(QStringLiteral("strokeWidth")).toInt(2);
    style.font.fromString(object.value(QStringLiteral("font")).toString());
    style.bold = object.value(QStringLiteral("bold")).toBool(false);
    style.italic = object.value(QStringLiteral("italic")).toBool(false);
    style.underline = object.value(QStringLiteral("underline")).toBool(false);
    style.rotation = object.value(QStringLiteral("rotation")).toDouble(0.0);
    style.scale = object.value(QStringLiteral("scale")).toDouble(1.0);
    return style;
}

QJsonObject itemToJson(const DiagramItemModel& item) {
    QJsonObject object{
        {QStringLiteral("id"), item.id},
        {QStringLiteral("typeId"), item.typeId},
        {QStringLiteral("kind"), item.kind() == ItemKind::Node ? QStringLiteral("node") : QStringLiteral("connector")},
        {QStringLiteral("text"), item.text},
        {QStringLiteral("zValue"), item.zValue},
        {QStringLiteral("style"), styleToJson(item.style)},
    };

    if (item.kind() == ItemKind::Node) {
        const auto& node = static_cast<const NodeModel&>(item);
        object.insert(QStringLiteral("rect"), rectToJson(node.rect));
        object.insert(QStringLiteral("props"), node.props);
    } else {
        const auto& connector = static_cast<const ConnectorModel&>(item);
        object.insert(QStringLiteral("start"), endpointToJson(connector.start));
        object.insert(QStringLiteral("end"), endpointToJson(connector.end));
        object.insert(QStringLiteral("props"), connector.props);
    }

    return object;
}

std::unique_ptr<DiagramItemModel> itemFromJson(const QJsonObject& object) {
    const QString kind = object.value(QStringLiteral("kind")).toString();
    if (kind == QStringLiteral("node")) {
        auto item = std::make_unique<NodeModel>();
        item->id = object.value(QStringLiteral("id")).toString();
        item->typeId = object.value(QStringLiteral("typeId")).toString();
        item->text = object.value(QStringLiteral("text")).toString();
        item->zValue = object.value(QStringLiteral("zValue")).toDouble(1.0);
        item->style = styleFromJson(object.value(QStringLiteral("style")));
        item->rect = rectFromJson(object.value(QStringLiteral("rect")));
        item->props = object.value(QStringLiteral("props")).toObject();
        return item;
    }

    auto item = std::make_unique<ConnectorModel>();
    item->id = object.value(QStringLiteral("id")).toString();
    item->typeId = object.value(QStringLiteral("typeId")).toString();
    item->text = object.value(QStringLiteral("text")).toString();
    item->zValue = object.value(QStringLiteral("zValue")).toDouble(1.0);
    item->style = styleFromJson(object.value(QStringLiteral("style")));
    item->start = endpointFromJson(object.value(QStringLiteral("start")));
    item->end = endpointFromJson(object.value(QStringLiteral("end")));
    item->props = object.value(QStringLiteral("props")).toObject();
    return item;
}

}  // namespace

QJsonObject DocumentSerializer::toJsonObject(const std::vector<std::unique_ptr<DiagramItemModel>>& items) {
    QJsonArray itemsArray;
    for (const auto& item : items) {
        itemsArray.push_back(itemToJson(*item));
    }

    return QJsonObject{
        {QStringLiteral("version"), 1},
        {QStringLiteral("items"), itemsArray},
    };
}

std::vector<std::unique_ptr<DiagramItemModel>> DocumentSerializer::fromJsonObject(
    const QJsonObject& root,
    QString* errorMessage) {
    const int version = root.value(QStringLiteral("version")).toInt(-1);
    if (version != 1) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Unsupported document version.");
        }
        return {};
    }

    std::vector<std::unique_ptr<DiagramItemModel>> items;
    const QJsonArray itemsArray = root.value(QStringLiteral("items")).toArray();
    items.reserve(itemsArray.size());
    for (const QJsonValue& value : itemsArray) {
        if (!value.isObject()) {
            continue;
        }
        items.push_back(itemFromJson(value.toObject()));
    }

    return items;
}

bool DocumentSerializer::save(
    const std::vector<std::unique_ptr<DiagramItemModel>>& items,
    const QString& fileName,
    QString* errorMessage) {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    const QJsonDocument document(toJsonObject(items));
    if (file.write(document.toJson(QJsonDocument::Indented)) == -1) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    return true;
}

std::vector<std::unique_ptr<DiagramItemModel>> DocumentSerializer::load(
    const QString& fileName,
    QString* errorMessage) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = parseError.errorString();
        }
        return {};
    }

    return fromJsonObject(document.object(), errorMessage);
}

}  // namespace flowchart
