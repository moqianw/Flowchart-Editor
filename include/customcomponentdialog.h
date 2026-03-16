#pragma once

#include <QColor>
#include <QDialog>
#include <QJsonObject>

class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;

namespace flowchart {

class CustomComponentDialog final : public QDialog {
    Q_OBJECT

public:
    explicit CustomComponentDialog(QWidget* parent = nullptr);

    QJsonObject componentDefinition() const;

protected:
    void accept() override;

private:
    void chooseStrokeColor();
    void chooseFillColor();
    void updateColorButton(QPushButton* button, const QColor& color) const;
    void loadSelectedTemplate();
    bool parseEditorObject(
        QPlainTextEdit* editor,
        const QString& fieldLabel,
        QJsonObject* outObject,
        QString* errorMessage) const;
    QJsonObject templateDrawSpec(const QString& templateId) const;
    QJsonObject templatePortsSpec(const QString& templateId) const;

    QLineEdit* m_typeIdEdit = nullptr;
    QLineEdit* m_labelEdit = nullptr;
    QComboBox* m_templateCombo = nullptr;
    QDoubleSpinBox* m_widthSpin = nullptr;
    QDoubleSpinBox* m_heightSpin = nullptr;
    QSpinBox* m_strokeWidthSpin = nullptr;
    QPushButton* m_strokeColorButton = nullptr;
    QPushButton* m_fillColorButton = nullptr;
    QPlainTextEdit* m_drawSpecEdit = nullptr;
    QPlainTextEdit* m_portsSpecEdit = nullptr;
    QColor m_strokeColor = QColor(0, 0, 0, 255);
    QColor m_fillColor = QColor(255, 255, 255, 0);
    QJsonObject m_componentDefinition;
};

}  // namespace flowchart
