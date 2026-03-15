#include "palettebutton.h"

#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>

namespace flowchart {

PaletteButton::PaletteButton(const QString& typeId, const QString& label, QWidget* parent)
    : QPushButton(label, parent)
    , m_typeId(typeId) {}

QString PaletteButton::typeId() const {
    return m_typeId;
}

void PaletteButton::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->pos();
    }
    QPushButton::mousePressEvent(event);
}

void PaletteButton::mouseMoveEvent(QMouseEvent* event) {
    if (!(event->buttons() & Qt::LeftButton)) {
        QPushButton::mouseMoveEvent(event);
        return;
    }

    if ((event->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance()) {
        QPushButton::mouseMoveEvent(event);
        return;
    }

    auto* drag = new QDrag(this);
    auto* mimeData = new QMimeData();
    mimeData->setData("application/x-flowchart-type", m_typeId.toUtf8());
    drag->setMimeData(mimeData);
    drag->setPixmap(icon().pixmap(iconSize()));
    drag->setHotSpot(event->pos());
    drag->exec(Qt::CopyAction);

    QPushButton::mouseMoveEvent(event);
}

}  // namespace flowchart
