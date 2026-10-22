#include "SyncManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QUrlQuery>
#include <QDebug>

SyncManager::SyncManager(SettingsManager *settings, PantryModel *pantry, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_pantry(pantry)
{
    if (m_settings) {
        connect(m_settings, &SettingsManager::syncUrlChanged, this, &SyncManager::configChanged);
        connect(m_settings, &SettingsManager::syncRoomIdChanged, this, &SyncManager::configChanged);
        connect(m_settings, &SettingsManager::syncTokenChanged, this, &SyncManager::configChanged);
    }
}

bool SyncManager::hasValidConfig() const {
    if (!m_settings) return false;
    return !m_settings->syncUrl().isEmpty() && !m_settings->syncRoomId().isEmpty();
}

QUrl SyncManager::buildRoomUrl() const {
    if (!m_settings) return QUrl();
    QString baseUrl = m_settings->syncUrl().trimmed();
    if (!baseUrl.startsWith("http://") && !baseUrl.startsWith("https://")) {
        baseUrl.prepend("https://");
    }
    if (baseUrl.endsWith('/')) {
        baseUrl.chop(1);
    }
    const QString roomId = m_settings->syncRoomId().trimmed();
    return QUrl(QString("%1/api/room/%2").arg(baseUrl, roomId));
}

void SyncManager::syncNow() {
    if (m_isSyncing) return;

    if (!hasValidConfig()) {
        m_lastSyncStatus = tr("Bitte Cloudflare URL und Raum-ID in den Einstellungen angeben.");
        emit lastSyncStatusChanged();
        emit syncFinished(false, m_lastSyncStatus);
        return;
    }

    m_isSyncing = true;
    emit isSyncingChanged();
    m_lastSyncStatus = tr("Synchronisiere mit Cloudflare Durable Object...");
    emit lastSyncStatusChanged();

    const QUrl url = buildRoomUrl();
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_settings->syncToken().isEmpty()) {
        request.setRawHeader("X-Pantry-Token", m_settings->syncToken().toUtf8());
    }

    // Build payload with local state
    QJsonObject payload;
    payload["roomId"] = m_settings->syncRoomId();
    payload["timestamp"] = QDateTime::currentSecsSinceEpoch();

    // Local pantry
    QJsonObject pantryObj;
    if (m_pantry) {
        const auto pMap = m_pantry->getPantryMap();
        for (auto it = pMap.constBegin(); it != pMap.constEnd(); ++it) {
            pantryObj[it.key()] = it.value();
        }
    }
    payload["pantry"] = pantryObj;

    // Starred recipes
    QJsonArray starredArr;
    if (m_settings) {
        for (const auto &id : m_settings->starredRecipeIds()) {
            starredArr.append(id);
        }
    }
    payload["starred"] = starredArr;

    const QByteArray data = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = m_networkManager.post(request, data);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleSyncReply(reply);
    });
}

void SyncManager::testConnection() {
    syncNow();
}

void SyncManager::handleSyncReply(QNetworkReply *reply) {
    reply->deleteLater();
    m_isSyncing = false;
    emit isSyncingChanged();

    if (reply->error() != QNetworkReply::NoError) {
        m_lastSyncStatus = tr("Verbindungsfehler: %1").arg(reply->errorString());
        emit lastSyncStatusChanged();
        emit syncFinished(false, m_lastSyncStatus);
        return;
    }

    const QByteArray responseData = reply->readAll();
    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        m_lastSyncStatus = tr("Server hat ungültiges JSON geantwortet.");
        emit lastSyncStatusChanged();
        emit syncFinished(false, m_lastSyncStatus);
        return;
    }

    const QJsonObject obj = doc.object();

    // Merge remote pantry state
    if (obj.contains("pantry") && obj.value("pantry").isObject() && m_pantry) {
        const QJsonObject remPantry = obj.value("pantry").toObject();
        QMap<QString, bool> currentMap = m_pantry->getPantryMap();
        for (auto it = remPantry.constBegin(); it != remPantry.constEnd(); ++it) {
            currentMap[it.key()] = it.value().toBool();
        }
        m_pantry->applyPantryMap(currentMap);
    }

    // Merge remote starred recipes
    if (obj.contains("starred") && obj.value("starred").isArray() && m_settings) {
        const QJsonArray remStarred = obj.value("starred").toArray();
        for (const auto &val : remStarred) {
            m_settings->setRecipeStarred(val.toString(), true);
        }
    }

    m_lastSyncTime = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss");
    m_lastSyncStatus = tr("Erfolgreich synchronisiert um %1").arg(m_lastSyncTime);
    emit lastSyncTimeChanged();
    emit lastSyncStatusChanged();
    emit syncFinished(true, m_lastSyncStatus);
}
