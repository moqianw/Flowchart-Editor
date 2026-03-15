#pragma once

#include <QPoint>
#include <QPushButton>
#include <QString>

namespace flowchart {

class PaletteButton final : public QPushButton {
    Q_OBJECT

public:
    PaletteButton(const QString& typeId, const QString& label, QWidget* parent = nullptr);

    QString typeId() const;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QString m_typeId;
    QPoint m_dragStartPos;
};

}  // namespace flowchart
