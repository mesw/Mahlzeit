#pragma once

#include <QObject>
#include <QSettings>
#include <QTime>
#include <QVariantList>
#include <QVariantMap>
#include <QSet>
#include <QMap>

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

public:
    explicit SettingsManager(QObject *parent = nullptr);

    Q_INVOKABLE void resetTimeSlotsToDefault();
    Q_INVOKABLE void updateTimeSlot(const QString &category, int startHour, int startMinute, int endHour, int endMinute);
    Q_INVOKABLE bool isCategoryActiveAtTime(const QString &category, const QTime &time) const;

    QVariantList timeSlotsVariant() const;
    QList<CategoryTimeSlot> timeSlots() const { return m_timeSlots; }

    // Starred recipes
    Q_INVOKABLE bool isRecipeStarred(const QString &recipeId) const;
    Q_INVOKABLE void setRecipeStarred(const QString &recipeId, bool starred);
    Q_INVOKABLE void toggleRecipeStarred(const QString &recipeId);
    QSet<QString> starredRecipeIds() const { return m_starredRecipeIds; }

    // Pantry persistence
    QMap<QString, bool> loadPantryMap() const;
    void savePantryMap(const QMap<QString, bool> &map);

    // Sync settings
    QString syncUrl() const { return m_syncUrl; }
    void setSyncUrl(const QString &url);

    QString syncRoomId() const { return m_syncRoomId; }
    void setSyncRoomId(const QString &roomId);

    QString syncToken() const { return m_syncToken; }
    void setSyncToken(const QString &token);

    bool autoSync() const { return m_autoSync; }
    void setAutoSync(bool enabled);

    // Factory reset
    Q_INVOKABLE void clearAllUserData();

signals:
    void timeSlotsChanged();
    void starredChanged(const QString &recipeId, bool starred);
    void syncUrlChanged();
    void syncRoomIdChanged();
    void syncTokenChanged();
    void autoSyncChanged();
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
    bool m_autoSync{false};
};
