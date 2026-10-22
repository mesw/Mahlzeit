#include "SettingsManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
    , m_settings("Malzeit", "Malzeit")
{
    loadSettings();
}

void SettingsManager::initializeDefaultSlots() {
    m_timeSlots.clear();
    // Default categories with time slots based on typical daily routines:
    m_timeSlots.append(CategoryTimeSlot{"Frühstück & Breie", 4, 0, 9, 30});
    m_timeSlots.append(CategoryTimeSlot{"Shakes", 7, 0, 18, 0});
    m_timeSlots.append(CategoryTimeSlot{"Aufstriche & Salate", 11, 0, 20, 0});
    m_timeSlots.append(CategoryTimeSlot{"Suppen & Nudelgerichte", 11, 0, 21, 30});
    m_timeSlots.append(CategoryTimeSlot{"Hauptgerichte", 11, 30, 21, 30});
    m_timeSlots.append(CategoryTimeSlot{"Süßspeisen", 12, 0, 22, 0});
}

void SettingsManager::loadSettings() {
    m_syncUrl = m_settings.value("sync/url", "").toString();
    m_syncRoomId = m_settings.value("sync/roomId", "").toString();
    m_syncToken = m_settings.value("sync/token", "").toString();
    m_autoSync = m_settings.value("sync/autoSync", false).toBool();

    // Starred recipes
    const QStringList starred = m_settings.value("starredRecipes").toStringList();
    m_starredRecipeIds = QSet<QString>(starred.begin(), starred.end());

    // Time slots
    const QString slotsJson = m_settings.value("timeSlotsJson").toString();
    if (!slotsJson.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(slotsJson.toUtf8());
        if (doc.isArray()) {
            m_timeSlots.clear();
            const QJsonArray arr = doc.array();
            for (const auto &val : arr) {
                const QJsonObject o = val.toObject();
                CategoryTimeSlot slot;
                slot.category = o.value("category").toString();
                slot.startHour = o.value("startHour").toInt(0);
                slot.startMinute = o.value("startMinute").toInt(0);
                slot.endHour = o.value("endHour").toInt(23);
                slot.endMinute = o.value("endMinute").toInt(59);
                m_timeSlots.append(slot);
            }
        }
    }

    if (m_timeSlots.isEmpty()) {
        initializeDefaultSlots();
    }
}

void SettingsManager::saveSettings() {
    m_settings.setValue("sync/url", m_syncUrl);
    m_settings.setValue("sync/roomId", m_syncRoomId);
    m_settings.setValue("sync/token", m_syncToken);
    m_settings.setValue("sync/autoSync", m_autoSync);

    QStringList starredList(m_starredRecipeIds.begin(), m_starredRecipeIds.end());
    m_settings.setValue("starredRecipes", starredList);

    QJsonArray arr;
    for (const auto &slot : m_timeSlots) {
        QJsonObject o;
        o["category"] = slot.category;
        o["startHour"] = slot.startHour;
        o["startMinute"] = slot.startMinute;
        o["endHour"] = slot.endHour;
        o["endMinute"] = slot.endMinute;
        arr.append(o);
    }
    const QJsonDocument doc(arr);
    m_settings.setValue("timeSlotsJson", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

    m_settings.sync();
}

void SettingsManager::resetTimeSlotsToDefault() {
    initializeDefaultSlots();
    saveSettings();
    emit timeSlotsChanged();
}

void SettingsManager::updateTimeSlot(const QString &category, int startHour, int startMinute, int endHour, int endMinute) {
    bool found = false;
    for (auto &slot : m_timeSlots) {
        if (slot.category == category) {
            slot.startHour = startHour;
            slot.startMinute = startMinute;
            slot.endHour = endHour;
            slot.endMinute = endMinute;
            found = true;
            break;
        }
    }
    if (!found) {
        m_timeSlots.append(CategoryTimeSlot{category, startHour, startMinute, endHour, endMinute});
    }
    saveSettings();
    emit timeSlotsChanged();
}

bool SettingsManager::isCategoryActiveAtTime(const QString &category, const QTime &time) const {
    for (const auto &slot : m_timeSlots) {
        if (slot.category.compare(category, Qt::CaseInsensitive) == 0) {
            return slot.isCurrentTimeActive(time);
        }
    }
    // If not found in slots, consider active so it isn't blocked by mistake
    return true;
}

QVariantList SettingsManager::timeSlotsVariant() const {
    QVariantList list;
    for (const auto &slot : m_timeSlots) {
        list.append(slot.toVariantMap());
    }
    return list;
}

bool SettingsManager::isRecipeStarred(const QString &recipeId) const {
    return m_starredRecipeIds.contains(recipeId);
}

void SettingsManager::setRecipeStarred(const QString &recipeId, bool starred) {
    if (recipeId.isEmpty()) return;
    const bool current = m_starredRecipeIds.contains(recipeId);
    if (current != starred) {
        if (starred) {
            m_starredRecipeIds.insert(recipeId);
        } else {
            m_starredRecipeIds.remove(recipeId);
        }
        saveSettings();
        emit starredChanged(recipeId, starred);
    }
}

void SettingsManager::toggleRecipeStarred(const QString &recipeId) {
    setRecipeStarred(recipeId, !isRecipeStarred(recipeId));
}

QMap<QString, bool> SettingsManager::loadPantryMap() const {
    QMap<QString, bool> result;
    const QString json = m_settings.value("pantryItemsJson").toString();
    if (!json.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        if (doc.isObject()) {
            const QJsonObject o = doc.object();
            for (auto it = o.constBegin(); it != o.constEnd(); ++it) {
                result[it.key()] = it.value().toBool();
            }
        }
    }
    return result;
}

void SettingsManager::savePantryMap(const QMap<QString, bool> &map) {
    QJsonObject obj;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        obj[it.key()] = it.value();
    }
    const QJsonDocument doc(obj);
    m_settings.setValue("pantryItemsJson", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    m_settings.sync();
}

void SettingsManager::setSyncUrl(const QString &url) {
    if (m_syncUrl != url) {
        m_syncUrl = url.trimmed();
        saveSettings();
        emit syncUrlChanged();
    }
}

void SettingsManager::setSyncRoomId(const QString &roomId) {
    if (m_syncRoomId != roomId) {
        m_syncRoomId = roomId.trimmed();
        saveSettings();
        emit syncRoomIdChanged();
    }
}

void SettingsManager::setSyncToken(const QString &token) {
    if (m_syncToken != token) {
        m_syncToken = token.trimmed();
        saveSettings();
        emit syncTokenChanged();
    }
}

void SettingsManager::setAutoSync(bool enabled) {
    if (m_autoSync != enabled) {
        m_autoSync = enabled;
        saveSettings();
        emit autoSyncChanged();
    }
}

void SettingsManager::clearAllUserData() {
    m_settings.clear();
    m_starredRecipeIds.clear();
    m_syncUrl.clear();
    m_syncRoomId.clear();
    m_syncToken.clear();
    m_autoSync = false;
    initializeDefaultSlots();
    saveSettings();
    emit timeSlotsChanged();
    emit syncUrlChanged();
    emit syncRoomIdChanged();
    emit syncTokenChanged();
    emit autoSyncChanged();
    emit userDataReset();
}
