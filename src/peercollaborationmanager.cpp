#include "peercollaborationmanager.h"

#include "documentserializer.h"
#include "editorsession.h"

#include <QDateTime>
#include <QHostAddress>
#include <QHostInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUuid>

namespace flowchart {

namespace {

constexpr int kProtocolVersion = 1;
constexpr int kSnapshotDebounceMs = 160;

QString trimmedOrFallback(const QString& value, const QString& fallback) {
    const QString trimmed = value.trimmed();
    return trimmed.isEmpty() ? fallback : trimmed;
}

}  // namespace

PeerCollaborationManager::PeerCollaborationManager(EditorSession* session, QObject* parent)
    : QObject(parent)
    , m_session(session)
    , m_server(new QTcpServer(this))
    , m_syncTimer(new QTimer(this))
    , m_localPeerId(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_localDisplayName(trimmedOrFallback(QHostInfo::localHostName(), QStringLiteral("Peer"))) {
    m_syncTimer->setSingleShot(true);
    m_syncTimer->setInterval(kSnapshotDebounceMs);

    connect(m_syncTimer, &QTimer::timeout, this, [this]() {
        if (!isConnected() || m_applyingRemoteSnapshot) {
            return;
        }
        sendSnapshot(QStringLiteral("document_changed"));
    });

    connect(m_session, &EditorSession::documentChanged, this, [this]() {
        if (m_applyingRemoteSnapshot) {
            return;
        }
        scheduleSnapshotSync();
    });

    connect(m_server, &QTcpServer::newConnection, this, [this]() {
        while (m_server->hasPendingConnections()) {
            QTcpSocket* socket = m_server->nextPendingConnection();
            if (m_socket) {
                socket->disconnectFromHost();
                socket->deleteLater();
                emit logMessage(QStringLiteral("拒绝额外连接：当前房间仅支持单个对端。"));
                continue;
            }

            attachSocket(socket, true);
            emit logMessage(QStringLiteral("已有成员加入房间，发送初始文档快照。"));
            sendHello();
            sendSnapshot(QStringLiteral("initial_sync"));
        }
    });
}

QString PeerCollaborationManager::localDisplayName() const {
    return m_localDisplayName;
}

void PeerCollaborationManager::setLocalDisplayName(const QString& name) {
    const QString nextName = trimmedOrFallback(name, QStringLiteral("Peer"));
    if (nextName == m_localDisplayName) {
        return;
    }
    m_localDisplayName = nextName;
    emit stateChanged();
    if (isConnected()) {
        sendHello();
    }
}

bool PeerCollaborationManager::startHosting(quint16 port, QString* errorMessage) {
    disconnectSession();
    if (!m_server->listen(QHostAddress::Any, port)) {
        if (errorMessage) {
            *errorMessage = m_server->errorString();
        }
        return false;
    }

    emit logMessage(QStringLiteral("房间已开放，监听端口 %1。").arg(m_server->serverPort()));
    emit stateChanged();
    return true;
}

bool PeerCollaborationManager::joinPeer(const QString& host, quint16 port, QString* errorMessage) {
    const QString trimmedHost = host.trimmed();
    if (trimmedHost.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("主机地址不能为空。");
        }
        return false;
    }

    disconnectSession();

    auto* socket = new QTcpSocket(this);
    attachSocket(socket, false);
    m_connecting = true;
    emit stateChanged();
    emit logMessage(QStringLiteral("正在连接 %1:%2 ...").arg(trimmedHost).arg(port));
    socket->connectToHost(trimmedHost, port);

    Q_UNUSED(errorMessage);
    return true;
}

void PeerCollaborationManager::disconnectSession() {
    m_syncTimer->stop();
    m_connecting = false;

    if (m_socket) {
        const bool wasConnected = m_socket->state() != QAbstractSocket::UnconnectedState;
        m_socket->blockSignals(true);
        if (wasConnected) {
            m_socket->disconnectFromHost();
            if (m_socket->state() != QAbstractSocket::UnconnectedState) {
                m_socket->abort();
            }
        }
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    if (m_server->isListening()) {
        m_server->close();
    }

    m_incomingBuffer.clear();
    m_remotePeerId.clear();
    m_remoteDisplayName.clear();
    m_lastReceivedRevision = 0;
    emit stateChanged();
}

bool PeerCollaborationManager::isHosting() const {
    return m_server->isListening();
}

bool PeerCollaborationManager::isConnected() const {
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

bool PeerCollaborationManager::isConnecting() const {
    return m_connecting;
}

quint16 PeerCollaborationManager::listeningPort() const {
    return m_server->isListening() ? m_server->serverPort() : 0;
}

QString PeerCollaborationManager::remoteDisplayName() const {
    return trimmedOrFallback(m_remoteDisplayName, QStringLiteral("-"));
}

QString PeerCollaborationManager::statusText() const {
    if (isConnected()) {
        return QStringLiteral("房间已连接：%1").arg(remoteDisplayName());
    }
    if (m_connecting) {
        return QStringLiteral("正在加入房间");
    }
    if (m_server->isListening()) {
        return QStringLiteral("已开房，等待加入（端口 %1）").arg(m_server->serverPort());
    }
    return QStringLiteral("未开房");
}

void PeerCollaborationManager::attachSocket(QTcpSocket* socket, bool acceptedIncoming) {
    detachSocket();
    m_socket = socket;
    m_socket->setParent(this);
    m_connecting = !acceptedIncoming;
    m_incomingBuffer.clear();
    m_remotePeerId.clear();
    m_remoteDisplayName.clear();
    m_lastReceivedRevision = 0;

    connect(m_socket, &QTcpSocket::connected, this, [this]() {
        m_connecting = false;
        emit logMessage(QStringLiteral("房间连接已建立。"));
        sendHello();
        emit stateChanged();
    });
    connect(m_socket, &QTcpSocket::readyRead, this, &PeerCollaborationManager::handleReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &PeerCollaborationManager::handleSocketDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        handleSocketError(QStringLiteral("房间连接错误"));
    });

    if (acceptedIncoming) {
        m_connecting = false;
        emit stateChanged();
    }
}

void PeerCollaborationManager::detachSocket() {
    if (!m_socket) {
        return;
    }
    m_socket->deleteLater();
    m_socket = nullptr;
    m_incomingBuffer.clear();
    m_remotePeerId.clear();
    m_remoteDisplayName.clear();
    m_lastReceivedRevision = 0;
}

void PeerCollaborationManager::sendJsonMessage(const QJsonObject& message) {
    if (!isConnected()) {
        return;
    }

    const QByteArray payload = QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n';
    m_socket->write(payload);
    m_socket->flush();
}

void PeerCollaborationManager::sendHello() {
    sendJsonMessage(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("hello")},
        {QStringLiteral("protocol"), kProtocolVersion},
        {QStringLiteral("peerId"), m_localPeerId},
        {QStringLiteral("displayName"), effectiveDisplayName()},
        {QStringLiteral("sentAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
    });
}

void PeerCollaborationManager::sendSnapshot(const QString& reason) {
    if (!isConnected()) {
        return;
    }

    m_localRevision += 1;
    sendJsonMessage(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("snapshot")},
        {QStringLiteral("protocol"), kProtocolVersion},
        {QStringLiteral("peerId"), m_localPeerId},
        {QStringLiteral("displayName"), effectiveDisplayName()},
        {QStringLiteral("revision"), QString::number(m_localRevision)},
        {QStringLiteral("reason"), reason},
        {QStringLiteral("document"), DocumentSerializer::toJsonObject(m_session->exportDocumentSnapshot())},
    });
}

void PeerCollaborationManager::scheduleSnapshotSync() {
    if (!isConnected()) {
        return;
    }
    m_syncTimer->start();
}

void PeerCollaborationManager::handleReadyRead() {
    if (!m_socket) {
        return;
    }

    m_incomingBuffer.append(m_socket->readAll());
    int newlineIndex = m_incomingBuffer.indexOf('\n');
    while (newlineIndex >= 0) {
        const QByteArray line = m_incomingBuffer.left(newlineIndex).trimmed();
        m_incomingBuffer.remove(0, newlineIndex + 1);
        newlineIndex = m_incomingBuffer.indexOf('\n');

        if (line.isEmpty()) {
            continue;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            emit logMessage(QStringLiteral("收到无法解析的协同消息：%1").arg(parseError.errorString()));
            continue;
        }
        processMessage(document.object());
    }
}

void PeerCollaborationManager::handleSocketDisconnected() {
    m_connecting = false;
    emit logMessage(QStringLiteral("房间连接已断开。"));

    if (m_socket) {
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    m_incomingBuffer.clear();
    m_remotePeerId.clear();
    m_remoteDisplayName.clear();
    m_lastReceivedRevision = 0;
    emit stateChanged();
}

void PeerCollaborationManager::handleSocketError(const QString& prefix) {
    if (!m_socket) {
        return;
    }
    emit logMessage(QStringLiteral("%1：%2").arg(prefix, m_socket->errorString()));
    emit stateChanged();
}

void PeerCollaborationManager::processMessage(const QJsonObject& message) {
    const QString type = message.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("hello")) {
        setRemotePeer(
            message.value(QStringLiteral("peerId")).toString(),
            message.value(QStringLiteral("displayName")).toString());
        emit logMessage(QStringLiteral("对端已加入房间：%1").arg(remoteDisplayName()));
        emit stateChanged();
        return;
    }

    if (type != QStringLiteral("snapshot")) {
        emit logMessage(QStringLiteral("忽略未知协同消息类型：%1").arg(type));
        return;
    }

    const QString peerId = message.value(QStringLiteral("peerId")).toString();
    if (!peerId.isEmpty() && peerId == m_localPeerId) {
        return;
    }

    setRemotePeer(peerId, message.value(QStringLiteral("displayName")).toString());

    bool revisionOk = false;
    const quint64 revision = message.value(QStringLiteral("revision")).toString().toULongLong(&revisionOk);
    if (revisionOk && revision <= m_lastReceivedRevision) {
        return;
    }

    QString error;
    auto items = DocumentSerializer::fromJsonObject(message.value(QStringLiteral("document")).toObject(), &error);
    if (!error.isEmpty()) {
        emit logMessage(QStringLiteral("应用房间快照失败：%1").arg(error));
        return;
    }

    m_applyingRemoteSnapshot = true;
    m_session->applyRemoteSnapshot(std::move(items));
    m_applyingRemoteSnapshot = false;
    if (revisionOk) {
        m_lastReceivedRevision = revision;
    }
    emit logMessage(QStringLiteral("已应用来自 %1 的房间同步更新。").arg(remoteDisplayName()));
    emit remoteSnapshotApplied();
    emit stateChanged();
}

void PeerCollaborationManager::setRemotePeer(const QString& peerId, const QString& displayName) {
    m_remotePeerId = peerId;
    m_remoteDisplayName = trimmedOrFallback(displayName, QStringLiteral("Peer"));
}

QString PeerCollaborationManager::effectiveDisplayName() const {
    return trimmedOrFallback(m_localDisplayName, QStringLiteral("Peer"));
}

}  // namespace flowchart
