#include "mermaidexporter.h"

#include <QFile>
#include <QHash>
#include <QStringConverter>
#include <QTextStream>

namespace flowchart {

namespace {

QString escapeLabel(QString label) {
    label.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    label.replace(QLatin1Char('\n'), QStringLiteral("<br/>"));
    return label;
}

QString nodeText(const NodeModel& node) {
    if (!node.text.trimmed().isEmpty()) {
        return node.text.trimmed();
    }
    return node.typeId;
}

QString mermaidNodeShape(const NodeModel& node, const QString& mermaidId) {
    const QString label = escapeLabel(nodeText(node));
    if (node.typeId == QStringLiteral("Diamond")) {
        return QStringLiteral("%1{\"%2\"}").arg(mermaidId, label);
    }
    if (node.typeId == QStringLiteral("Start_or_Terminator")) {
        return QStringLiteral("%1([\"%2\"])").arg(mermaidId, label);
    }
    if (node.typeId == QStringLiteral("Ellipse")) {
        return QStringLiteral("%1((\"%2\"))").arg(mermaidId, label);
    }
    return QStringLiteral("%1[\"%2\"]").arg(mermaidId, label);
}

}  // namespace

bool MermaidExporter::save(const DiagramDocument& document, const QString& fileName, QString* errorMessage) {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << "flowchart TD\n";

    QHash<QString, QString> mermaidIds;
    int nodeIndex = 1;
    for (const QString& itemId : document.itemIds()) {
        const auto* node = document.node(itemId);
        if (!node) {
            continue;
        }
        const QString mermaidId = QStringLiteral("n%1").arg(nodeIndex++);
        mermaidIds.insert(itemId, mermaidId);
        stream << "    " << mermaidNodeShape(*node, mermaidId) << '\n';
    }

    stream << '\n';
    for (const QString& itemId : document.itemIds()) {
        const auto* connector = document.connector(itemId);
        if (!connector || !connector->start.isAttached() || !connector->end.isAttached()) {
            continue;
        }

        const QString startId = mermaidIds.value(connector->start.itemId);
        const QString endId = mermaidIds.value(connector->end.itemId);
        if (startId.isEmpty() || endId.isEmpty()) {
            continue;
        }

        stream << "    " << startId << " --> " << endId << '\n';
    }

    if (stream.status() != QTextStream::Ok) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to write Mermaid file.");
        }
        return false;
    }

    return true;
}

}  // namespace flowchart
