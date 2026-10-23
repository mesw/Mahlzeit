#pragma once

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>
#include "SettingsManager.h"
#include "models/PantryModel.h"

class SyncManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isSyncing READ isSyncing NOTIFY isSyncingChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString lastSyncTime READ lastSyncTime NOTIFY lastSyncTimeChanged)
    Q_PROPERTY(QString lastSyncStatus READ lastSyncStatus NOTIFY lastSyncStatusChanged)
    Q_PROPERTY(bool hasValidConfig READ hasValidConfig NOTIFY configChanged)

public:
    explicit SyncManager(SettingsManager *settings, PantryModel *pantry, QObject *parent = nullptr);

    bool isSyncing() const { return m_isSyncing; }
    bool isConnected() const { return m_connected; }
    QString lastSyncTime() const { return m_lastSyncTime; }
    QString lastSyncStatus() const { return m_lastSyncStatus; }
    bool hasValidConfig() const;

    Q_INVOKABLE void connectToServer();
    Q_INVOKABLE void disconnectFromServer();
    Q_INVOKABLE void syncNow();
    Q_INVOKABLE void pullNow();
    Q_INVOKABLE void pushDelta(bool includeFullSeed = false);
    Q_INVOKABLE void testConnection();

signals:
    void isSyncingChanged();
    void connectedChanged();
    void lastSyncTimeChanged();
    void lastSyncStatusChanged();
    void configChanged();
    void syncFinished(bool success, const QString &message);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onTextMessageReceived(const QString &message);
    void onSocketError(QAbstractSocket::SocketError error);
    void updateConnectionState();

private:
    QUrl buildWebSocketUrl() const;
    bool hasPendingDeltas() const;
    void scheduleDeltaPush();

    SettingsManager *m_settings{nullptr};
    PantryModel *m_pantry{nullptr};
    QWebSocket m_socket;
    QTimer m_reconnectTimer;
    QTimer m_configDebounceTimer;
    QTimer m_pushDebounceTimer;

    QMap<QString, TimedDeltaEntry> m_pendingPantryDelta;
    QMap<QString, TimedDeltaEntry> m_pendingStarredDelta;
    QString m_pendingStoreId;
    qint64 m_pendingStoreTimestampMs{0};

    bool m_isSyncing{false};
    bool m_connected{false};
    bool m_authFailed{false};
    bool m_applyingRemoteState{false};
    QUrl m_activeSocketUrl;
    QString m_lastSyncTime;
    QString m_lastSyncStatus{tr("Nicht synchronisiert")};
};
