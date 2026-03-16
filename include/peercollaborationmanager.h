#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QString>

class QTcpServer;
class QTcpSocket;
class QTimer;

namespace flowchart {

class EditorSession;

class PeerCollaborationManager final : public QObject {
    Q_OBJECT

public:
    explicit PeerCollaborationManager(EditorSession* session, QObject* parent = nullptr);

    QString localDisplayName() const;
    void setLocalDisplayName(const QString& name);

    bool startHosting(quint16 port, QString* errorMessage = nullptr);
    bool joinPeer(const QString& host, quint16 port, QString* errorMessage = nullptr);
    void disconnectSession();

    bool isHosting() const;
    bool isConnected() const;
    bool isConnecting() const;
    quint16 listeningPort() const;
    QString remoteDisplayName() const;
    QString statusText() const;

signals:
    void stateChanged();
    void logMessage(const QString& message);
    void remoteSnapshotApplied();

private:
    void attachSocket(QTcpSocket* socket, bool acceptedIncoming);
    void detachSocket();
    void sendJsonMessage(const QJsonObject& message);
    void sendHello();
    void sendSnapshot(const QString& reason);
    void scheduleSnapshotSync();
    void handleReadyRead();
    void handleSocketDisconnected();
    void handleSocketError(const QString& prefix);
    void processMessage(const QJsonObject& message);
    void setRemotePeer(const QString& peerId, const QString& displayName);
    QString effectiveDisplayName() const;

    EditorSession* m_session;
    QTcpServer* m_server = nullptr;
    QTcpSocket* m_socket = nullptr;
    QTimer* m_syncTimer = nullptr;
    QByteArray m_incomingBuffer;
    QString m_localPeerId;
    QString m_localDisplayName;
    QString m_remotePeerId;
    QString m_remoteDisplayName;
    quint64 m_localRevision = 0;
    quint64 m_lastReceivedRevision = 0;
    bool m_applyingRemoteSnapshot = false;
    bool m_connecting = false;
};

}  // namespace flowchart
