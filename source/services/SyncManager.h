#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "SettingsManager.h"
#include "models/PantryModel.h"

class SyncManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isSyncing READ isSyncing NOTIFY isSyncingChanged)
    Q_PROPERTY(QString lastSyncTime READ lastSyncTime NOTIFY lastSyncTimeChanged)
    Q_PROPERTY(QString lastSyncStatus READ lastSyncStatus NOTIFY lastSyncStatusChanged)
    Q_PROPERTY(bool hasValidConfig READ hasValidConfig NOTIFY configChanged)

public:
    explicit SyncManager(SettingsManager *settings, PantryModel *pantry, QObject *parent = nullptr);

    bool isSyncing() const { return m_isSyncing; }
    QString lastSyncTime() const { return m_lastSyncTime; }
    QString lastSyncStatus() const { return m_lastSyncStatus; }
    bool hasValidConfig() const;

    Q_INVOKABLE void syncNow();
    Q_INVOKABLE void testConnection();

signals:
    void isSyncingChanged();
    void lastSyncTimeChanged();
    void lastSyncStatusChanged();
    void configChanged();
    void syncFinished(bool success, const QString &message);

private slots:
    void handleSyncReply(QNetworkReply *reply);

private:
    QUrl buildRoomUrl() const;

    SettingsManager *m_settings{nullptr};
    PantryModel *m_pantry{nullptr};
    QNetworkAccessManager m_networkManager;
    bool m_isSyncing{false};
    QString m_lastSyncTime;
    QString m_lastSyncStatus{tr("Nicht synchronisiert")};
};
