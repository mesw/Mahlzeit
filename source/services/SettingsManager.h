#pragma once

#include <QObject>
#include <QSettings>
#include <QTime>
#include <QVariantList>
#include <QVariantMap>
#include <QSet>
#include <QMap>
#include <QPair>

struct TimedDeltaEntry {
    bool value{false};
    qint64 timestampMs{0};
};

struct CategoryTimeSlot {
    QString category;
    int startHour{0};
    int startMinute{0};
    int endHour{23};
    int endMinute{59};

    bool isCurrentTimeActive(const QTime &now = QTime::currentTime()) const {
        const QTime start(startHour, startMinute);
        const QTime end(endHour, endMinute);
        if (start <= end) {
            return now >= start && now <= end;
        }
        // Overnight slot (e.g. 22:00 - 04:00)
        return now >= start || now <= end;
    }

    QVariantMap toVariantMap() const {
        return QVariantMap{
            {"category", category},
            {"startHour", startHour},
            {"startMinute", startMinute},
            {"endHour", endHour},
            {"endMinute", endMinute},
            {"timeRangeString", QString("%1:%2 - %3:%4")
                .arg(startHour, 2, 10, QChar('0'))
                .arg(startMinute, 2, 10, QChar('0'))
                .arg(endHour, 2, 10, QChar('0'))
                .arg(endMinute, 2, 10, QChar('0'))}
        };
    }
};

class SettingsManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList timeSlots READ timeSlotsVariant NOTIFY timeSlotsChanged)
    Q_PROPERTY(QString syncUrl READ syncUrl WRITE setSyncUrl NOTIFY syncUrlChanged)
    Q_PROPERTY(QString syncRoomId READ syncRoomId WRITE setSyncRoomId NOTIFY syncRoomIdChanged)
    Q_PROPERTY(QString syncToken READ syncToken WRITE setSyncToken NOTIFY syncTokenChanged)
    Q_PROPERTY(bool autoSync READ autoSync WRITE setAutoSync NOTIFY autoSyncChanged)
    Q_PROPERTY(QString activeStoreId READ activeStoreId WRITE setActiveStoreId NOTIFY activeStoreIdChanged)
    Q_PROPERTY(QString recipesUrl READ recipesUrl WRITE setRecipesUrl NOTIFY recipesUrlChanged)

public:
    explicit SettingsManager(QObject *parent = nullptr);

    static QString defaultRecipesUrl();

    Q_INVOKABLE void resetTimeSlotsToDefault();
    Q_INVOKABLE void updateTimeSlot(const QString &category, int startHour, int startMinute, int endHour, int endMinute);
    Q_INVOKABLE bool isCategoryActiveAtTime(const QString &category, const QTime &time) const;

    QVariantList timeSlotsVariant() const;
    QList<CategoryTimeSlot> timeSlots() const { return m_timeSlots; }

    // Starred recipes
    Q_INVOKABLE bool isRecipeStarred(const QString &recipeId) const;
    Q_INVOKABLE void setRecipeStarred(const QString &recipeId, bool starred);
    Q_INVOKABLE void toggleRecipeStarred(const QString &recipeId);
    void applyRemoteStarredRecipes(const QSet<QString> &remoteStarred);
    QSet<QString> starredRecipeIds() const { return m_starredRecipeIds; }

    // Pantry persistence
    QMap<QString, bool> loadPantryMap() const;
    void savePantryMap(const QMap<QString, bool> &map);

    // Persistent Offline Delta Queue (LWW CRDT)
    QMap<QString, TimedDeltaEntry> loadPendingPantryDelta() const;
    void savePendingPantryDelta(const QMap<QString, TimedDeltaEntry> &delta);
    QMap<QString, TimedDeltaEntry> loadPendingStarredDelta() const;
    void savePendingStarredDelta(const QMap<QString, TimedDeltaEntry> &delta);
    QPair<QString, qint64> loadPendingStoreDelta() const;
    void savePendingStoreDelta(const QString &storeId, qint64 timestampMs);

    // Sync settings
    QString syncUrl() const { return m_syncUrl; }
    void setSyncUrl(const QString &url);

    QString syncRoomId() const { return m_syncRoomId; }
    void setSyncRoomId(const QString &roomId);

    QString syncToken() const { return m_syncToken; }
    void setSyncToken(const QString &token);

    bool autoSync() const { return m_autoSync; }
    void setAutoSync(bool enabled);

    // Store & Catalog settings
    QString activeStoreId() const { return m_activeStoreId; }
    void setActiveStoreId(const QString &storeId);
    void applyRemoteActiveStoreId(const QString &storeId);

    QString recipesUrl() const { return m_recipesUrl; }
    void setRecipesUrl(const QString &url);

    // Factory reset
    Q_INVOKABLE void clearAllUserData();

signals:
    void timeSlotsChanged();
    void starredChanged(const QString &recipeId, bool starred);
    void starredRecipeEdited(const QString &recipeId, bool starred);
    void syncUrlChanged();
    void syncRoomIdChanged();
    void syncTokenChanged();
    void autoSyncChanged();
    void activeStoreIdChanged();
    void activeStoreEdited(const QString &storeId);
    void recipesUrlChanged();
    void userDataReset();

private:
    void loadSettings();
    void saveSettings();
    void initializeDefaultSlots();

    QSettings m_settings;
    QList<CategoryTimeSlot> m_timeSlots;
    QSet<QString> m_starredRecipeIds;
    QString m_syncUrl;
    QString m_syncRoomId;
    QString m_syncToken;
    bool m_autoSync{true};
    QString m_activeStoreId;
    QString m_recipesUrl;
};
