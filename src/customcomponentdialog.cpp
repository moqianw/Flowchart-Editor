#include "customcomponentdialog.h"

#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointF>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QVBoxLayout>

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

QString toJsonText(const QJsonObject& object) {
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Indented));
}

QJsonObject colorToJson(const QColor& color) {
    return QJsonObject{
        {QStringLiteral("r"), color.red()},
        {QStringLiteral("g"), color.green()},
        {QStringLiteral("b"), color.blue()},
        {QStringLiteral("a"), color.alpha()},
    };
}

}  // namespace

CustomComponentDialog::CustomComponentDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("新建自定义组件"));
    setModal(true);
    resize(760, 720);

    auto* rootLayout = new QVBoxLayout(this);
    auto* formLayout = new QFormLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(8);

    m_typeIdEdit = new QLineEdit(this);
    m_typeIdEdit->setPlaceholderText(QStringLiteral("例如：CustomApprovalNode"));
    m_labelEdit = new QLineEdit(this);
    m_labelEdit->setPlaceholderText(QStringLiteral("例如：审批节点"));

    m_templateCombo = new QComboBox(this);
    m_templateCombo->addItem(QStringLiteral("矩形模板"), QStringLiteral("rectangle"));
    m_templateCombo->addItem(QStringLiteral("菱形模板"), QStringLiteral("diamond"));
    m_templateCombo->addItem(QStringLiteral("六边形模板"), QStringLiteral("hexagon"));
    m_templateCombo->addItem(QStringLiteral("数据库模板"), QStringLiteral("database"));
    m_templateCombo->addItem(QStringLiteral("文档模板"), QStringLiteral("document"));
    m_templateCombo->addItem(QStringLiteral("空白模板"), QStringLiteral("blank"));

    auto* loadTemplateButton = new QPushButton(QStringLiteral("载入模板"), this);
    connect(loadTemplateButton, &QPushButton::clicked, this, [this]() { loadSelectedTemplate(); });

    auto* templateRow = new QWidget(this);
    auto* templateLayout = new QHBoxLayout(templateRow);
    templateLayout->setContentsMargins(0, 0, 0, 0);
    templateLayout->addWidget(m_templateCombo, 1);
    templateLayout->addWidget(loadTemplateButton);

    m_widthSpin = new QDoubleSpinBox(this);
    m_widthSpin->setRange(40.0, 1000.0);
    m_widthSpin->setValue(140.0);
    m_widthSpin->setSingleStep(10.0);
    m_heightSpin = new QDoubleSpinBox(this);
    m_heightSpin->setRange(30.0, 600.0);
    m_heightSpin->setValue(90.0);
    m_heightSpin->setSingleStep(10.0);

    m_strokeWidthSpin = new QSpinBox(this);
    m_strokeWidthSpin->setRange(1, 16);
    m_strokeWidthSpin->setValue(2);

    m_strokeColorButton = new QPushButton(this);
    m_fillColorButton = new QPushButton(this);
    updateColorButton(m_strokeColorButton, m_strokeColor);
    updateColorButton(m_fillColorButton, m_fillColor);

    m_drawSpecEdit = new QPlainTextEdit(this);
    m_drawSpecEdit->setPlaceholderText(QStringLiteral("{\n  \"commands\": [ ... ]\n}"));
    m_drawSpecEdit->setMinimumHeight(260);
    m_portsSpecEdit = new QPlainTextEdit(this);
    m_portsSpecEdit->setPlaceholderText(QStringLiteral("{\n  \"kind\": \"fourWay\"\n}"));
    m_portsSpecEdit->setMinimumHeight(130);

    formLayout->addRow(QStringLiteral("组件 ID"), m_typeIdEdit);
    formLayout->addRow(QStringLiteral("显示名称"), m_labelEdit);
    formLayout->addRow(QStringLiteral("模板"), templateRow);
    formLayout->addRow(QStringLiteral("默认宽度"), m_widthSpin);
    formLayout->addRow(QStringLiteral("默认高度"), m_heightSpin);
    formLayout->addRow(QStringLiteral("边框线宽"), m_strokeWidthSpin);
    formLayout->addRow(QStringLiteral("边框颜色"), m_strokeColorButton);
    formLayout->addRow(QStringLiteral("填充颜色"), m_fillColorButton);

    auto* drawLabel = new QLabel(QStringLiteral("drawSpec"), this);
    QFont sectionFont = drawLabel->font();
    sectionFont.setBold(true);
    drawLabel->setFont(sectionFont);

    auto* drawHint = new QLabel(
        QStringLiteral("统一绘制格式。支持的 op: moveTo, lineTo, quadTo, cubicTo, rect, roundedRect, ellipse, polygon, arcMoveTo, arcTo, close, parallelogram。坐标默认按 0~1 归一化到组件边界。"),
        this);
    drawHint->setWordWrap(true);
    drawHint->setStyleSheet(QStringLiteral("color: #666;"));

    auto* portsLabel = new QLabel(QStringLiteral("ports"), this);
    portsLabel->setFont(sectionFont);

    auto* portsHint = new QLabel(
        QStringLiteral("端口可写 {\"kind\":\"fourWay\"} / twoVertical / twoHorizontal / none，或 {\"kind\":\"custom\",\"points\":[{\"x\":0.5,\"y\":0.0}, ...]}。"),
        this);
    portsHint->setWordWrap(true);
    portsHint->setStyleSheet(QStringLiteral("color: #666;"));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &CustomComponentDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &CustomComponentDialog::reject);
    connect(m_strokeColorButton, &QPushButton::clicked, this, [this]() { chooseStrokeColor(); });
    connect(m_fillColorButton, &QPushButton::clicked, this, [this]() { chooseFillColor(); });

    rootLayout->addLayout(formLayout);
    rootLayout->addSpacing(6);
    rootLayout->addWidget(drawLabel);
    rootLayout->addWidget(drawHint);
    rootLayout->addWidget(m_drawSpecEdit, 1);
    rootLayout->addSpacing(6);
    rootLayout->addWidget(portsLabel);
    rootLayout->addWidget(portsHint);
    rootLayout->addWidget(m_portsSpecEdit);
    rootLayout->addWidget(buttons);

    loadSelectedTemplate();
}

QJsonObject CustomComponentDialog::componentDefinition() const {
    return m_componentDefinition;
}

void CustomComponentDialog::accept() {
    const QString typeId = m_typeIdEdit->text().trimmed();
    const QString label = m_labelEdit->text().trimmed();
    if (typeId.isEmpty() || label.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("信息不完整"), QStringLiteral("组件 ID 和显示名称不能为空。"));
        return;
    }

    static const QRegularExpression typeIdPattern(QStringLiteral("^[A-Za-z_][A-Za-z0-9_-]*$"));
    if (!typeIdPattern.match(typeId).hasMatch()) {
        QMessageBox::warning(
            this,
            QStringLiteral("组件 ID 无效"),
            QStringLiteral("组件 ID 仅支持字母、数字、下划线和连字符，且必须以字母或下划线开头。"));
        return;
    }

    QJsonObject drawSpecObject;
    QString error;
    if (!parseEditorObject(m_drawSpecEdit, QStringLiteral("drawSpec"), &drawSpecObject, &error)) {
        QMessageBox::warning(this, QStringLiteral("drawSpec 无效"), error);
        return;
    }
    if (drawSpecObject.value(QStringLiteral("commands")).toArray().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("drawSpec 无效"), QStringLiteral("drawSpec.commands 不能为空。"));
        return;
    }

    QJsonObject portsObject;
    if (!m_portsSpecEdit->toPlainText().trimmed().isEmpty()
        && !parseEditorObject(m_portsSpecEdit, QStringLiteral("ports"), &portsObject, &error)) {
        QMessageBox::warning(this, QStringLiteral("ports 无效"), error);
        return;
    }

    m_componentDefinition = QJsonObject{
        {QStringLiteral("typeId"), typeId},
        {QStringLiteral("paletteLabel"), label},
        {QStringLiteral("defaultSize"), QJsonObject{
            {QStringLiteral("w"), m_widthSpin->value()},
            {QStringLiteral("h"), m_heightSpin->value()},
        }},
        {QStringLiteral("drawSpec"), drawSpecObject},
        {QStringLiteral("ports"), portsObject.isEmpty() ? templatePortsSpec(QStringLiteral("rectangle")) : portsObject},
        {QStringLiteral("style"), QJsonObject{
            {QStringLiteral("strokeColor"), colorToJson(m_strokeColor)},
            {QStringLiteral("fillColor"), colorToJson(m_fillColor)},
            {QStringLiteral("strokeWidth"), m_strokeWidthSpin->value()},
        }},
    };

    QDialog::accept();
}

void CustomComponentDialog::chooseStrokeColor() {
    const QColor color = QColorDialog::getColor(
        m_strokeColor,
        this,
        QStringLiteral("选择边框颜色"),
        QColorDialog::ShowAlphaChannel);
    if (!color.isValid()) {
        return;
    }
    m_strokeColor = color;
    updateColorButton(m_strokeColorButton, m_strokeColor);
}

void CustomComponentDialog::chooseFillColor() {
    const QColor color = QColorDialog::getColor(
        m_fillColor,
        this,
        QStringLiteral("选择填充颜色"),
        QColorDialog::ShowAlphaChannel);
    if (!color.isValid()) {
        return;
    }
    m_fillColor = color;
    updateColorButton(m_fillColorButton, m_fillColor);
}

void CustomComponentDialog::updateColorButton(QPushButton* button, const QColor& color) const {
    if (!button) {
        return;
    }
    button->setText(color.name(QColor::HexArgb));
    button->setStyleSheet(QStringLiteral(
        "text-align: left; padding: 4px 8px; background: %1; color: %2;")
            .arg(color.name(QColor::HexArgb))
            .arg(color.lightnessF() < 0.45 ? QStringLiteral("#ffffff") : QStringLiteral("#000000")));
}

void CustomComponentDialog::loadSelectedTemplate() {
    const QString templateId = m_templateCombo->currentData().toString();
    m_drawSpecEdit->setPlainText(toJsonText(templateDrawSpec(templateId)));
    m_portsSpecEdit->setPlainText(toJsonText(templatePortsSpec(templateId)));
}

bool CustomComponentDialog::parseEditorObject(
    QPlainTextEdit* editor,
    const QString& fieldLabel,
    QJsonObject* outObject,
    QString* errorMessage) const {
    if (!editor || !outObject) {
        return false;
    }

    const QByteArray bytes = editor->toPlainText().trimmed().toUtf8();
    if (bytes.isEmpty()) {
        *outObject = QJsonObject{};
        return true;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("%1 必须是合法的 JSON 对象：%2").arg(fieldLabel, parseError.errorString());
        }
        return false;
    }

    *outObject = document.object();
    return true;
}

QJsonObject CustomComponentDialog::templateDrawSpec(const QString& templateId) const {
    if (templateId == QStringLiteral("diamond")) {
        return jsonObject({
            {QStringLiteral("commands"), QJsonArray{
                command(QStringLiteral("polygon"), {
                    {QStringLiteral("points"), pointsToJson({
                        QPointF(0.5, 0.0),
                        QPointF(0.0, 0.5),
                        QPointF(0.5, 1.0),
                        QPointF(1.0, 0.5),
                    })},
                }),
            }},
        });
    }
    if (templateId == QStringLiteral("hexagon")) {
        return jsonObject({
            {QStringLiteral("commands"), QJsonArray{
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
            }},
        });
    }
    if (templateId == QStringLiteral("database")) {
        return jsonObject({
            {QStringLiteral("commands"), QJsonArray{
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
            }},
        });
    }
    if (templateId == QStringLiteral("document")) {
        return jsonObject({
            {QStringLiteral("commands"), QJsonArray{
                command(QStringLiteral("moveTo"), {{QStringLiteral("x"), 0.0}, {QStringLiteral("y"), 0.85}}),
                command(QStringLiteral("lineTo"), {{QStringLiteral("x"), 0.0}, {QStringLiteral("y"), 0.0}}),
                command(QStringLiteral("lineTo"), {{QStringLiteral("x"), 1.0}, {QStringLiteral("y"), 0.0}}),
                command(QStringLiteral("lineTo"), {{QStringLiteral("x"), 1.0}, {QStringLiteral("y"), 0.85}}),
                command(QStringLiteral("arcTo"), {
                    {QStringLiteral("x"), 0.0},
                    {QStringLiteral("y"), 0.7},
                    {QStringLiteral("w"), 0.5},
                    {QStringLiteral("h"), 0.3},
                    {QStringLiteral("start"), 180.0},
                    {QStringLiteral("sweep"), 180.0},
                }),
                command(QStringLiteral("arcTo"), {
                    {QStringLiteral("x"), 0.5},
                    {QStringLiteral("y"), 0.7},
                    {QStringLiteral("w"), 0.5},
                    {QStringLiteral("h"), 0.3},
                    {QStringLiteral("start"), 0.0},
                    {QStringLiteral("sweep"), 180.0},
                }),
            }},
        });
    }
    if (templateId == QStringLiteral("blank")) {
        return jsonObject({
            {QStringLiteral("commands"), QJsonArray{
                command(QStringLiteral("moveTo"), {{QStringLiteral("x"), 0.1}, {QStringLiteral("y"), 0.1}}),
                command(QStringLiteral("lineTo"), {{QStringLiteral("x"), 0.9}, {QStringLiteral("y"), 0.1}}),
                command(QStringLiteral("lineTo"), {{QStringLiteral("x"), 0.9}, {QStringLiteral("y"), 0.9}}),
                command(QStringLiteral("lineTo"), {{QStringLiteral("x"), 0.1}, {QStringLiteral("y"), 0.9}}),
                command(QStringLiteral("close")),
            }},
        });
    }

    return jsonObject({
        {QStringLiteral("commands"), QJsonArray{
            command(QStringLiteral("rect")),
        }},
    });
}

QJsonObject CustomComponentDialog::templatePortsSpec(const QString& templateId) const {
    if (templateId == QStringLiteral("document")) {
        return jsonObject({{QStringLiteral("kind"), QStringLiteral("twoVertical")}});
    }
    if (templateId == QStringLiteral("blank")) {
        return jsonObject({
            {QStringLiteral("kind"), QStringLiteral("custom")},
            {QStringLiteral("points"), pointsToJson({
                QPointF(0.5, 0.0),
                QPointF(1.0, 0.5),
                QPointF(0.5, 1.0),
                QPointF(0.0, 0.5),
            })},
        });
    }
    return jsonObject({{QStringLiteral("kind"), QStringLiteral("fourWay")}});
}

}  // namespace flowchart
