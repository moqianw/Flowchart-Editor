#include "diagramtypes.h"

#include <QUuid>

namespace flowchart {

bool EndpointRef::isAttached() const {
    return !itemId.isEmpty() && portIndex >= 0;
}

ItemKind NodeModel::kind() const {
    return ItemKind::Node;
}

std::unique_ptr<DiagramItemModel> NodeModel::clone() const {
    return std::make_unique<NodeModel>(*this);
}

ItemKind ConnectorModel::kind() const {
    return ItemKind::Connector;
}

std::unique_ptr<DiagramItemModel> ConnectorModel::clone() const {
    return std::make_unique<ConnectorModel>(*this);
}

QString createItemId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

namespace {

bool colorsEqual(const QColor& left, const QColor& right) {
    return left.rgba() == right.rgba();
}

bool fontsEqual(const QFont& left, const QFont& right) {
    return left.toString() == right.toString();
}

bool stylesEqual(const ItemStyle& left, const ItemStyle& right) {
    return colorsEqual(left.strokeColor, right.strokeColor)
        && colorsEqual(left.fillColor, right.fillColor)
        && left.strokeWidth == right.strokeWidth
        && fontsEqual(left.font, right.font)
        && left.bold == right.bold
        && left.italic == right.italic
        && left.underline == right.underline
        && qFuzzyCompare(left.rotation + 1.0, right.rotation + 1.0)
        && qFuzzyCompare(left.scale + 1.0, right.scale + 1.0);
}

bool endpointsEqual(const EndpointRef& left, const EndpointRef& right) {
    return left.itemId == right.itemId
        && left.portIndex == right.portIndex
        && left.position == right.position;
}

}  // namespace

bool modelsEqual(const DiagramItemModel& left, const DiagramItemModel& right) {
    if (left.kind() != right.kind()) {
        return false;
    }
    if (left.id != right.id
        || left.typeId != right.typeId
        || left.text != right.text
        || !qFuzzyCompare(left.zValue + 1.0, right.zValue + 1.0)
        || !stylesEqual(left.style, right.style)) {
        return false;
    }

    if (left.kind() == ItemKind::Node) {
        const auto& nodeLeft = static_cast<const NodeModel&>(left);
        const auto& nodeRight = static_cast<const NodeModel&>(right);
        return nodeLeft.rect == nodeRight.rect && nodeLeft.props == nodeRight.props;
    }

    const auto& connectorLeft = static_cast<const ConnectorModel&>(left);
    const auto& connectorRight = static_cast<const ConnectorModel&>(right);
    return endpointsEqual(connectorLeft.start, connectorRight.start)
        && endpointsEqual(connectorLeft.end, connectorRight.end)
        && connectorLeft.props == connectorRight.props;
}

std::vector<std::unique_ptr<DiagramItemModel>> cloneModels(
    const std::vector<std::unique_ptr<DiagramItemModel>>& items) {
    std::vector<std::unique_ptr<DiagramItemModel>> result;
    result.reserve(items.size());
    for (const auto& item : items) {
        result.push_back(item->clone());
    }
    return result;
}

}  // namespace flowchart
