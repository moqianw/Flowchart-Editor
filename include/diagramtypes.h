#pragma once

#include <QColor>
#include <QFont>
#include <QJsonObject>
#include <QPointF>
#include <QRectF>
#include <QString>

#include <memory>
#include <vector>

namespace flowchart {

enum class ItemKind {
    Node,
    Connector,
};

struct ItemStyle {
    QColor strokeColor = QColor(0, 0, 0, 255);
    QColor fillColor = QColor(255, 255, 255, 0);
    int strokeWidth = 2;
    QFont font = QFont(QStringLiteral("Microsoft YaHei"), 12);
    bool bold = false;
    bool italic = false;
    bool underline = false;
    qreal rotation = 0.0;
    qreal scale = 1.0;
};

struct EndpointRef {
    QString itemId;
    int portIndex = -1;
    QPointF position;

    bool isAttached() const;
};

class DiagramItemModel {
public:
    virtual ~DiagramItemModel() = default;

    QString id;
    QString typeId;
    QString text;
    qreal zValue = 1.0;
    ItemStyle style;

    virtual ItemKind kind() const = 0;
    virtual std::unique_ptr<DiagramItemModel> clone() const = 0;
};

class NodeModel final : public DiagramItemModel {
public:
    QRectF rect;
    QJsonObject props;

    ItemKind kind() const override;
    std::unique_ptr<DiagramItemModel> clone() const override;
};

class ConnectorModel final : public DiagramItemModel {
public:
    EndpointRef start;
    EndpointRef end;
    QJsonObject props;

    ItemKind kind() const override;
    std::unique_ptr<DiagramItemModel> clone() const override;
};

QString createItemId();
bool modelsEqual(const DiagramItemModel& left, const DiagramItemModel& right);
std::vector<std::unique_ptr<DiagramItemModel>> cloneModels(
    const std::vector<std::unique_ptr<DiagramItemModel>>& items);

}  // namespace flowchart
