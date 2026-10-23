#include "SyncManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDateTime>
#include <QGuiApplication>
#include <QDebug>

static constexpr int RECONNECT_INTERVAL_MS = 3000; // Auto-reconnect every 3 seconds
static constexpr int CONFIG_DEBOUNCE_MS = 700;     // Wait briefly while typing in Settings
static constexpr int PUSH_DEBOUNCE_MS = 200;       // Coalesce rapid local item toggles

SyncManager::SyncManager(SettingsManager *settings, PantryModel *pantry, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_pantry(pantry)
{
    connect(&m_socket, &QWebSocket::connected, this, &SyncManager::onSocketConnected);
    connect(&m_socket, &QWebSocket::disconnected, this, &SyncManager::onSocketDisconnected);
    connect(&m_socket, &QWebSocket::textMessageReceived, this, &SyncManager::onTextMessageReceived);
    connect(&m_socket, &QWebSocket::errorOccurred, this, &SyncManager::onSocketError);

    m_reconnectTimer.setSingleShot(true);
    m_reconnectTimer.setInterval(RECONNECT_INTERVAL_MS);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &SyncManager::connectToServer);

    m_configDebounceTimer.setSingleShot(true);
    m_configDebounceTimer.setInterval(CONFIG_DEBOUNCE_MS);
    connect(&m_configDebounceTimer, &QTimer::timeout, this, &SyncManager::updateConnectionState);

    m_pushDebounceTimer.setSingleShot(true);
    m_pushDebounceTimer.setInterval(PUSH_DEBOUNCE_MS);
    connect(&m_pushDebounceTimer, &QTimer::timeout, this, [this]() {
        pushDelta(false);
    });

    if (m_settings) {
        // Restore any unsent offline edits from persistent storage (QSettings / localStorage)
        m_pendingPantryDelta = m_settings->loadPendingPantryDelta();
        m_pendingStarredDelta = m_settings->loadPendingStarredDelta();
        const auto storeDelta = m_settings->loadPendingStoreDelta();
        m_pendingStoreId = storeDelta.first;
        m_pendingStoreTimestampMs = storeDelta.second;

        auto onConfigEdited = [this]() {
            m_authFailed = false;
            emit configChanged();
            m_configDebounceTimer.start();
        };
        connect(m_settings, &SettingsManager::syncUrlChanged, this, onConfigEdited);
        connect(m_settings, &SettingsManager::syncRoomIdChanged, this, onConfigEdited);
        connect(m_settings, &SettingsManager::syncTokenChanged, this, onConfigEdited);
        connect(m_settings, &SettingsManager::autoSyncChanged, this, &SyncManager::updateConnectionState);

        connect(m_settings, &SettingsManager::starredRecipeEdited, this, [this](const QString &recipeId, bool starred) {
            if (m_applyingRemoteState || recipeId.isEmpty()) return;
            m_pendingStarredDelta[recipeId] = TimedDeltaEntry{starred, QDateTime::currentMSecsSinceEpoch()};
            if (m_settings) {
                m_settings->savePendingStarredDelta(m_pendingStarredDelta);
            }
            scheduleDeltaPush();
        });

        connect(m_settings, &SettingsManager::activeStoreEdited, this, [this](const QString &storeId) {
            if (m_applyingRemoteState || storeId.isEmpty()) return;
            m_pendingStoreId = storeId;
            m_pendingStoreTimestampMs = QDateTime::currentMSecsSinceEpoch();
            if (m_settings) {
                m_settings->savePendingStoreDelta(m_pendingStoreId, m_pendingStoreTimestampMs);
            }
            scheduleDeltaPush();
        });

        connect(m_settings, &SettingsManager::userDataReset, this, [this]() {
            m_pendingPantryDelta.clear();
            m_pendingStarredDelta.clear();
            m_pendingStoreId.clear();
            m_pendingStoreTimestampMs = 0;
            disconnectFromServer();
        });
    }

    if (m_pantry) {
        connect(m_pantry, &PantryModel::itemAvailabilityEdited, this, [this](const QString &canonicalName, bool available) {
            if (m_applyingRemoteState || canonicalName.isEmpty()) return;
            m_pendingPantryDelta[canonicalName] = TimedDeltaEntry{available, QDateTime::currentMSecsSinceEpoch()};
            if (m_settings) {
                m_settings->savePendingPantryDelta(m_pendingPantryDelta);
            }
            scheduleDeltaPush();
        });

        connect(m_pantry, &PantryModel::bulkAvailabilityEdited, this, [this](const QMap<QString, bool> &changes) {
            if (m_applyingRemoteState || changes.isEmpty()) return;
            const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
            for (auto it = changes.constBegin(); it != changes.constEnd(); ++it) {
                m_pendingPantryDelta[it.key()] = TimedDeltaEntry{it.value(), nowMs};
            }
            if (m_settings) {
                m_settings->savePendingPantryDelta(m_pendingPantryDelta);
            }
            scheduleDeltaPush();
        });
    }

    if (auto *guiApp = qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        connect(guiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
            if (state == Qt::ApplicationActive && hasValidConfig() && (!m_settings || m_settings->autoSync())) {
                if (!m_connected) {
                    connectToServer();
                } else if (hasPendingDeltas()) {
                    pushDelta(false);
                }
            }
        });
    }

    // Connect automatically on startup if configured
    QTimer::singleShot(300, this, &SyncManager::updateConnectionState);
}

bool SyncManager::hasValidConfig() const {
    if (!m_settings) return false;
    return !m_settings->syncUrl().trimmed().isEmpty() && !m_settings->syncRoomId().trimmed().isEmpty();
}

bool SyncManager::hasPendingDeltas() const {
    return !m_pendingPantryDelta.isEmpty() || !m_pendingStarredDelta.isEmpty() || !m_pendingStoreId.isEmpty();
}

QUrl SyncManager::buildWebSocketUrl() const {
    if (!m_settings) return QUrl();

    QString baseUrl = m_settings->syncUrl().trimmed();
    if (baseUrl.isEmpty()) return QUrl();

    if (baseUrl.startsWith("https://", Qt::CaseInsensitive)) {
        baseUrl = "wss://" + baseUrl.mid(8);
    } else if (baseUrl.startsWith("http://", Qt::CaseInsensitive)) {
        baseUrl = "ws://" + baseUrl.mid(7);
    } else if (!baseUrl.startsWith("wss://", Qt::CaseInsensitive) &&
               !baseUrl.startsWith("ws://", Qt::CaseInsensitive)) {
        const bool isLocal = baseUrl.startsWith("localhost") || baseUrl.startsWith("127.0.0.1");
        baseUrl.prepend(isLocal ? "ws://" : "wss://");
    }

    while (baseUrl.endsWith('/')) {
        baseUrl.chop(1);
    }

    // In case the user pasted a full URL ending with /api/room/xyz, strip that suffix first
    const int roomPathIdx = baseUrl.indexOf("/api/room/", 0, Qt::CaseInsensitive);
    if (roomPathIdx != -1) {
        baseUrl = baseUrl.left(roomPathIdx);
    }

    const QString roomId = m_settings->syncRoomId().trimmed();
    QUrl url(QString("%1/api/room/%2").arg(baseUrl, roomId));

    const QString token = m_settings->syncToken().trimmed();
    if (!token.isEmpty()) {
        QUrlQuery query;
        query.addQueryItem("token", token);
        url.setQuery(query);
    }

    return url;
}

void SyncManager::updateConnectionState() {
    if (!hasValidConfig()) {
        disconnectFromServer();
        return;
    }

    if (m_settings && !m_settings->autoSync()) {
        m_reconnectTimer.stop();
        return;
    }

    const QUrl targetUrl = buildWebSocketUrl();
    if (m_connected && m_activeSocketUrl == targetUrl) {
        return;
    }

    connectToServer();
}

void SyncManager::connectToServer() {
    m_reconnectTimer.stop();

    if (!hasValidConfig()) {
        m_lastSyncStatus = tr("Bitte Cloudflare URL und Raum-ID in den Einstellungen angeben.");
        emit lastSyncStatusChanged();
        emit syncFinished(false, m_lastSyncStatus);
        return;
    }

    const QUrl targetUrl = buildWebSocketUrl();
    if (!targetUrl.isValid()) {
        m_lastSyncStatus = tr("Ungültige Worker-URL.");
        emit lastSyncStatusChanged();
        emit syncFinished(false, m_lastSyncStatus);
        return;
    }

    if (m_socket.state() == QAbstractSocket::ConnectedState ||
        m_socket.state() == QAbstractSocket::ConnectingState) {
        if (m_activeSocketUrl == targetUrl) {
            return;
        }
        m_socket.abort();
    }

    m_activeSocketUrl = targetUrl;
    m_isSyncing = true;
    emit isSyncingChanged();
    m_lastSyncStatus = tr("Verbinde mit Raum '%1'...").arg(m_settings->syncRoomId().trimmed());
    emit lastSyncStatusChanged();

    m_socket.open(targetUrl);
}

void SyncManager::disconnectFromServer() {
    m_reconnectTimer.stop();
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.close();
    }
    if (m_connected) {
        m_connected = false;
        emit connectedChanged();
    }
    if (m_isSyncing) {
        m_isSyncing = false;
        emit isSyncingChanged();
    }
}

void SyncManager::scheduleDeltaPush() {
    if (!hasValidConfig()) return;
    if (m_settings && !m_settings->autoSync()) return;
    m_pushDebounceTimer.start();
}

void SyncManager::pullNow() {
    if (!hasValidConfig()) {
        m_lastSyncStatus = tr("Bitte Cloudflare URL und Raum-ID in den Einstellungen angeben.");
        emit lastSyncStatusChanged();
        emit syncFinished(false, m_lastSyncStatus);
        return;
    }

    if (!m_connected) {
        connectToServer();
        return;
    }

    if (hasPendingDeltas()) {
        pushDelta(false);
        return;
    }

    m_isSyncing = true;
    emit isSyncingChanged();

    QJsonObject msg;
    msg["type"] = QStringLiteral("pull");
    m_socket.sendTextMessage(QString::fromUtf8(QJsonDocument(msg).toJson(QJsonDocument::Compact)));
}

void SyncManager::pushDelta(bool includeFullSeed) {
    if (!hasValidConfig()) {
        m_lastSyncStatus = tr("Bitte Cloudflare URL und Raum-ID in den Einstellungen angeben.");
        emit lastSyncStatusChanged();
        emit syncFinished(false, m_lastSyncStatus);
        return;
    }

    if (!m_connected) {
        connectToServer();
        return;
    }

    m_isSyncing = true;
    emit isSyncingChanged();

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    QJsonObject payload;
    payload["type"] = QStringLiteral("delta");
    payload["roomId"] = m_settings->syncRoomId().trimmed();
    payload["timestamp"] = nowMs;

    // Per-item timestamped LWW delta objects: { "Apfel": { "value": true, "ts": 1729700000000 } }
    QJsonObject pantryDeltaObj;
    for (auto it = m_pendingPantryDelta.constBegin(); it != m_pendingPantryDelta.constEnd(); ++it) {
        QJsonObject entry;
        entry["value"] = it.value().value;
        entry["ts"] = it.value().timestampMs;
        pantryDeltaObj[it.key()] = entry;
    }
    payload["pantryDelta"] = pantryDeltaObj;

    QJsonObject starredDeltaObj;
    for (auto it = m_pendingStarredDelta.constBegin(); it != m_pendingStarredDelta.constEnd(); ++it) {
        QJsonObject entry;
        entry["value"] = it.value().value;
        entry["ts"] = it.value().timestampMs;
        starredDeltaObj[it.key()] = entry;
    }
    payload["starredDelta"] = starredDeltaObj;

    if (!m_pendingStoreId.isEmpty()) {
        payload["activeStoreId"] = m_pendingStoreId;
        payload["storeTimestamp"] = m_pendingStoreTimestampMs;
    } else if (includeFullSeed && m_settings) {
        payload["activeStoreId"] = m_settings->activeStoreId();
        payload["storeTimestamp"] = nowMs;
    }

    if (includeFullSeed) {
        QJsonObject pantrySeedObj;
        if (m_pantry) {
            const auto pMap = m_pantry->getPantryMap();
            for (auto it = pMap.constBegin(); it != pMap.constEnd(); ++it) {
                pantrySeedObj[it.key()] = it.value();
            }
        }
        payload["pantry"] = pantrySeedObj;

        QJsonArray starredSeedArr;
        if (m_settings) {
            for (const auto &id : m_settings->starredRecipeIds()) {
                starredSeedArr.append(id);
            }
        }
        payload["starred"] = starredSeedArr;
    }

    // Note: We do NOT clear m_pendingPantryDelta / m_pendingStarredDelta here before the server
    // acknowledges them in onTextMessageReceived. That guarantees offline deltas are never lost
    // if the connection drops mid-send or if an older initial "state" frame arrives first.
    m_socket.sendTextMessage(QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact)));
}

void SyncManager::syncNow() {
    m_authFailed = false;
    m_configDebounceTimer.stop();

    const QUrl targetUrl = buildWebSocketUrl();
    if (!m_connected || m_activeSocketUrl != targetUrl) {
        connectToServer();
        return;
    }

    pushDelta(true);
}

void SyncManager::testConnection() {
    syncNow();
}

void SyncManager::onSocketConnected() {
    m_connected = true;
    emit connectedChanged();
    m_reconnectTimer.stop();

    m_lastSyncStatus = tr("Verbunden — Synchronisiere...");
    emit lastSyncStatusChanged();

    // If the user queued local edits while offline, flush them now
    if (hasPendingDeltas()) {
        pushDelta(true);
    }
}

void SyncManager::onSocketDisconnected() {
    if (m_connected) {
        m_connected = false;
        emit connectedChanged();
    }
    if (m_isSyncing) {
        m_isSyncing = false;
        emit isSyncingChanged();
    }

    if (hasValidConfig() && (!m_settings || m_settings->autoSync()) && !m_authFailed) {
        if (hasPendingDeltas()) {
            m_lastSyncStatus = tr("Offline — %1 Änderung(en) in Warteschlange")
                .arg(m_pendingPantryDelta.size() + m_pendingStarredDelta.size() + (m_pendingStoreId.isEmpty() ? 0 : 1));
            emit lastSyncStatusChanged();
        }
        m_reconnectTimer.start();
    }
}

void SyncManager::onSocketError(QAbstractSocket::SocketError error) {
    Q_UNUSED(error)
    m_isSyncing = false;
    emit isSyncingChanged();

    if (!m_authFailed) {
        if (hasPendingDeltas()) {
            m_lastSyncStatus = tr("Offline (%1 Änderung(en) gespeichert) — Verbinde neu...")
                .arg(m_pendingPantryDelta.size() + m_pendingStarredDelta.size() + (m_pendingStoreId.isEmpty() ? 0 : 1));
        } else {
            m_lastSyncStatus = tr("Verbindungsfehler: %1").arg(m_socket.errorString());
        }
        emit lastSyncStatusChanged();
        emit syncFinished(false, m_lastSyncStatus);
    }
}

void SyncManager::onTextMessageReceived(const QString &message) {
    m_isSyncing = false;
    emit isSyncingChanged();

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    const QJsonObject obj = doc.object();

    if (obj.value("type").toString() == QLatin1String("error") || obj.contains("error")) {
        const QString errMsg = obj.value("error").toString(tr("Unbekannter Serverfehler"));
        if (errMsg.contains("Unauthorized", Qt::CaseInsensitive)) {
            m_authFailed = true;
            m_reconnectTimer.stop();
        }
        m_lastSyncStatus = tr("Sync-Fehler: %1").arg(errMsg);
        emit lastSyncStatusChanged();
        emit syncFinished(false, m_lastSyncStatus);
        return;
    }

    const QJsonObject remPantry = obj.value("pantry").toObject();
    const QJsonObject remPantryTs = obj.value("pantryTimestamps").toObject();
    const QJsonArray remStarred = obj.value("starred").toArray();
    const QJsonObject remStarredTs = obj.value("starredTimestamps").toObject();
    const qint64 remStoreTs = obj.value("storeTimestamp").toVariant().toLongLong();

    const bool remoteIsEmpty = remPantry.isEmpty() && remStarred.isEmpty();
    const bool localHasData = (m_pantry && m_pantry->availableCount() > 0) ||
                              (m_settings && !m_settings->starredRecipeIds().isEmpty());

    // If this is a brand-new empty room on the server and we have local data, seed it once
    if (remoteIsEmpty && localHasData) {
        pushDelta(true);
        return;
    }

    // Acknowledge and remove any pending deltas whose timestamp has been reached or superseded on the server
    bool pantryQueueModified = false;
    for (auto it = m_pendingPantryDelta.begin(); it != m_pendingPantryDelta.end(); ) {
        const qint64 serverItemTs = remPantryTs.value(it.key()).toVariant().toLongLong();
        if (serverItemTs >= it.value().timestampMs) {
            it = m_pendingPantryDelta.erase(it);
            pantryQueueModified = true;
        } else {
            ++it;
        }
    }
    if (pantryQueueModified && m_settings) {
        m_settings->savePendingPantryDelta(m_pendingPantryDelta);
    }

    bool starredQueueModified = false;
    for (auto it = m_pendingStarredDelta.begin(); it != m_pendingStarredDelta.end(); ) {
        const qint64 serverItemTs = remStarredTs.value(it.key()).toVariant().toLongLong();
        if (serverItemTs >= it.value().timestampMs) {
            it = m_pendingStarredDelta.erase(it);
            starredQueueModified = true;
        } else {
            ++it;
        }
    }
    if (starredQueueModified && m_settings) {
        m_settings->savePendingStarredDelta(m_pendingStarredDelta);
    }

    if (!m_pendingStoreId.isEmpty() && remStoreTs >= m_pendingStoreTimestampMs) {
        m_pendingStoreId.clear();
        m_pendingStoreTimestampMs = 0;
        if (m_settings) {
            m_settings->savePendingStoreDelta(QString(), 0);
        }
    }

    m_applyingRemoteState = true;

    // Apply authoritative remote pantry state, overlaid with any unacknowledged newer local deltas
    if (obj.contains("pantry") && obj.value("pantry").isObject() && m_pantry) {
        if (!remPantry.isEmpty()) {
            QMap<QString, bool> mergedMap = m_pantry->getPantryMap();
            for (auto it = remPantry.constBegin(); it != remPantry.constEnd(); ++it) {
                mergedMap[it.key()] = it.value().toBool();
            }
            for (auto it = m_pendingPantryDelta.constBegin(); it != m_pendingPantryDelta.constEnd(); ++it) {
                mergedMap[it.key()] = it.value().value;
            }
            m_pantry->applyPantryMap(mergedMap);
        }
    }

    // Apply authoritative remote starred recipes, overlaid with any unacknowledged newer local deltas
    if (obj.contains("starred") && obj.value("starred").isArray() && m_settings) {
        QSet<QString> remoteSet;
        for (const auto &val : remStarred) {
            const QString id = val.toString().trimmed();
            if (!id.isEmpty()) {
                remoteSet.insert(id);
            }
        }
        for (auto it = m_pendingStarredDelta.constBegin(); it != m_pendingStarredDelta.constEnd(); ++it) {
            if (it.value().value) {
                remoteSet.insert(it.key());
            } else {
                remoteSet.remove(it.key());
            }
        }
        m_settings->applyRemoteStarredRecipes(remoteSet);
    }

    // Apply remote active store if no newer local store delta is pending
    if (obj.contains("activeStoreId") && obj.value("activeStoreId").isString() && m_settings && m_pendingStoreId.isEmpty()) {
        const QString remoteStore = obj.value("activeStoreId").toString().trimmed();
        if (!remoteStore.isEmpty()) {
            m_settings->applyRemoteActiveStoreId(remoteStore);
        }
    }

    m_applyingRemoteState = false;

    m_lastSyncTime = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_lastSyncStatus = tr("🟢 Live verbunden (%1)").arg(m_lastSyncTime);
    emit lastSyncTimeChanged();
    emit lastSyncStatusChanged();
    emit syncFinished(true, m_lastSyncStatus);

    // If we still have unacknowledged local deltas (e.g. after receiving the initial state on connect), flush them
    if (hasPendingDeltas() && !m_pushDebounceTimer.isActive()) {
        m_pushDebounceTimer.start();
    }
}
