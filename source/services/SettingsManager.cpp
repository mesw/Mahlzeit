#include "SettingsManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <cstdlib>

static QString wasmLocalStorageGet(const char *key) {
    char *str = reinterpret_cast<char *>(EM_ASM_PTR({
        try {
            var k = UTF8ToString($0);
            var v = window.localStorage.getItem(k);
            if (v === null || v === undefined) return 0;
            return stringToNewUTF8(v);
        } catch (e) {
            return 0;
        }
    }, key));
    if (!str) return QString();
    QString result = QString::fromUtf8(str);
    std::free(str);
    return result;
}

static void wasmLocalStorageSet(const char *key, const QString &value) {
    const QByteArray utf8 = value.toUtf8();
    EM_ASM({
        try {
            window.localStorage.setItem(UTF8ToString($0), UTF8ToString($1));
        } catch (e) {}
    }, key, utf8.constData());
}

static void wasmLocalStorageRemove(const char *key) {
    EM_ASM({
        try {
            window.localStorage.removeItem(UTF8ToString($0));
        } catch (e) {}
    }, key);
}
#endif

QString SettingsManager::defaultRecipesUrl() {
    return QStringLiteral("https://mesw.github.io/Malzeit/resources/recipes.json");
}

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
#ifdef __EMSCRIPTEN__
    const QString wasmConfigJson = wasmLocalStorageGet("malzeit_settings_v1");
    if (!wasmConfigJson.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(wasmConfigJson.toUtf8());
        if (doc.isObject()) {
            const QJsonObject cfg = doc.object();
            if (cfg.contains("syncUrl")) m_settings.setValue("sync/url", cfg.value("syncUrl").toString());
            if (cfg.contains("syncRoomId")) m_settings.setValue("sync/roomId", cfg.value("syncRoomId").toString());
            if (cfg.contains("syncToken")) m_settings.setValue("sync/token", cfg.value("syncToken").toString());
            if (cfg.contains("autoSync")) m_settings.setValue("sync/autoSync", cfg.value("autoSync").toBool(true));
            if (cfg.contains("activeStoreId")) m_settings.setValue("store/activeStoreId", cfg.value("activeStoreId").toString());
            if (cfg.contains("recipesUrl")) m_settings.setValue("catalog/recipesUrl", cfg.value("recipesUrl").toString());
            if (cfg.contains("starredRecipes") && cfg.value("starredRecipes").isArray()) {
                QStringList sList;
                for (const auto &v : cfg.value("starredRecipes").toArray()) sList.append(v.toString());
                m_settings.setValue("starredRecipes", sList);
            }
            if (cfg.contains("timeSlotsJson")) {
                m_settings.setValue("timeSlotsJson", cfg.value("timeSlotsJson").toString());
            }
        }
    }
#endif
    m_syncUrl = m_settings.value("sync/url", "").toString();
    m_syncRoomId = m_settings.value("sync/roomId", "").toString();
    m_syncToken = m_settings.value("sync/token", "").toString();
    m_autoSync = m_settings.value("sync/autoSync", true).toBool();
    m_activeStoreId = m_settings.value("store/activeStoreId", "stamm-supermarkt").toString();
    m_recipesUrl = m_settings.value("catalog/recipesUrl", defaultRecipesUrl()).toString();
    if (m_recipesUrl.trimmed().isEmpty()) {
        m_recipesUrl = defaultRecipesUrl();
    }

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
    m_settings.setValue("store/activeStoreId", m_activeStoreId);
    m_settings.setValue("catalog/recipesUrl", m_recipesUrl);

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
    const QString slotsJsonStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    m_settings.setValue("timeSlotsJson", slotsJsonStr);

    m_settings.sync();

#ifdef __EMSCRIPTEN__
    QJsonObject cfg;
    cfg["syncUrl"] = m_syncUrl;
    cfg["syncRoomId"] = m_syncRoomId;
    cfg["syncToken"] = m_syncToken;
    cfg["autoSync"] = m_autoSync;
    cfg["activeStoreId"] = m_activeStoreId;
    cfg["recipesUrl"] = m_recipesUrl;
    QJsonArray starredArr;
    for (const auto &s : starredList) starredArr.append(s);
    cfg["starredRecipes"] = starredArr;
    cfg["timeSlotsJson"] = slotsJsonStr;
    wasmLocalStorageSet("malzeit_settings_v1", QString::fromUtf8(QJsonDocument(cfg).toJson(QJsonDocument::Compact)));
#endif
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
        emit starredRecipeEdited(recipeId, starred);
    }
}

void SettingsManager::toggleRecipeStarred(const QString &recipeId) {
    setRecipeStarred(recipeId, !isRecipeStarred(recipeId));
}

void SettingsManager::applyRemoteStarredRecipes(const QSet<QString> &remoteStarred) {
    if (m_starredRecipeIds == remoteStarred) {
        return;
    }

    const QSet<QString> added = remoteStarred - m_starredRecipeIds;
    const QSet<QString> removed = m_starredRecipeIds - remoteStarred;
    m_starredRecipeIds = remoteStarred;
    saveSettings();

    for (const QString &id : added) {
        emit starredChanged(id, true);
    }
    for (const QString &id : removed) {
        emit starredChanged(id, false);
    }
}

QMap<QString, bool> SettingsManager::loadPantryMap() const {
    QMap<QString, bool> result;
    QString json = m_settings.value("pantryItemsJson").toString();
#ifdef __EMSCRIPTEN__
    if (json.isEmpty()) {
        json = wasmLocalStorageGet("malzeit_pantry_v1");
    }
#endif
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
    const QString jsonStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    m_settings.setValue("pantryItemsJson", jsonStr);
    m_settings.sync();
#ifdef __EMSCRIPTEN__
    wasmLocalStorageSet("malzeit_pantry_v1", jsonStr);
#endif
}

static QMap<QString, TimedDeltaEntry> parseDeltaJson(const QString &json) {
    QMap<QString, TimedDeltaEntry> result;
    if (json.isEmpty()) return result;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) return result;
    const QJsonObject o = doc.object();
    for (auto it = o.constBegin(); it != o.constEnd(); ++it) {
        if (it.value().isObject()) {
            const QJsonObject entryObj = it.value().toObject();
            TimedDeltaEntry entry;
            entry.value = entryObj.value("value").toBool();
            entry.timestampMs = entryObj.value("ts").toVariant().toLongLong();
            result[it.key()] = entry;
        }
    }
    return result;
}

static QString serializeDeltaMap(const QMap<QString, TimedDeltaEntry> &delta) {
    QJsonObject obj;
    for (auto it = delta.constBegin(); it != delta.constEnd(); ++it) {
        QJsonObject entryObj;
        entryObj["value"] = it.value().value;
        entryObj["ts"] = it.value().timestampMs;
        obj[it.key()] = entryObj;
    }
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QMap<QString, TimedDeltaEntry> SettingsManager::loadPendingPantryDelta() const {
    QString json = m_settings.value("sync/pendingPantryDeltaJson").toString();
#ifdef __EMSCRIPTEN__
    if (json.isEmpty()) {
        json = wasmLocalStorageGet("malzeit_pending_pantry_delta_v1");
    }
#endif
    return parseDeltaJson(json);
}

void SettingsManager::savePendingPantryDelta(const QMap<QString, TimedDeltaEntry> &delta) {
    const QString jsonStr = delta.isEmpty() ? QString() : serializeDeltaMap(delta);
    m_settings.setValue("sync/pendingPantryDeltaJson", jsonStr);
    m_settings.sync();
#ifdef __EMSCRIPTEN__
    if (jsonStr.isEmpty()) {
        wasmLocalStorageRemove("malzeit_pending_pantry_delta_v1");
    } else {
        wasmLocalStorageSet("malzeit_pending_pantry_delta_v1", jsonStr);
    }
#endif
}

QMap<QString, TimedDeltaEntry> SettingsManager::loadPendingStarredDelta() const {
    QString json = m_settings.value("sync/pendingStarredDeltaJson").toString();
#ifdef __EMSCRIPTEN__
    if (json.isEmpty()) {
        json = wasmLocalStorageGet("malzeit_pending_starred_delta_v1");
    }
#endif
    return parseDeltaJson(json);
}

void SettingsManager::savePendingStarredDelta(const QMap<QString, TimedDeltaEntry> &delta) {
    const QString jsonStr = delta.isEmpty() ? QString() : serializeDeltaMap(delta);
    m_settings.setValue("sync/pendingStarredDeltaJson", jsonStr);
    m_settings.sync();
#ifdef __EMSCRIPTEN__
    if (jsonStr.isEmpty()) {
        wasmLocalStorageRemove("malzeit_pending_starred_delta_v1");
    } else {
        wasmLocalStorageSet("malzeit_pending_starred_delta_v1", jsonStr);
    }
#endif
}

QPair<QString, qint64> SettingsManager::loadPendingStoreDelta() const {
    QString json = m_settings.value("sync/pendingStoreDeltaJson").toString();
#ifdef __EMSCRIPTEN__
    if (json.isEmpty()) {
        json = wasmLocalStorageGet("malzeit_pending_store_delta_v1");
    }
#endif
    if (!json.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        if (doc.isObject()) {
            const QJsonObject o = doc.object();
            return qMakePair(o.value("storeId").toString(), o.value("ts").toVariant().toLongLong());
        }
    }
    return qMakePair(QString(), 0LL);
}

void SettingsManager::savePendingStoreDelta(const QString &storeId, qint64 timestampMs) {
    if (storeId.isEmpty() || timestampMs <= 0) {
        m_settings.setValue("sync/pendingStoreDeltaJson", QString());
        m_settings.sync();
#ifdef __EMSCRIPTEN__
        wasmLocalStorageRemove("malzeit_pending_store_delta_v1");
#endif
        return;
    }
    QJsonObject o;
    o["storeId"] = storeId;
    o["ts"] = timestampMs;
    const QString jsonStr = QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
    m_settings.setValue("sync/pendingStoreDeltaJson", jsonStr);
    m_settings.sync();
#ifdef __EMSCRIPTEN__
    wasmLocalStorageSet("malzeit_pending_store_delta_v1", jsonStr);
#endif
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

void SettingsManager::setActiveStoreId(const QString &storeId) {
    const QString trimmed = storeId.trimmed();
    if (m_activeStoreId != trimmed) {
        m_activeStoreId = trimmed;
        saveSettings();
        emit activeStoreIdChanged();
        emit activeStoreEdited(m_activeStoreId);
    }
}

void SettingsManager::applyRemoteActiveStoreId(const QString &storeId) {
    const QString trimmed = storeId.trimmed();
    if (!trimmed.isEmpty() && m_activeStoreId != trimmed) {
        m_activeStoreId = trimmed;
        saveSettings();
        emit activeStoreIdChanged();
    }
}

void SettingsManager::setRecipesUrl(const QString &url) {
    QString trimmed = url.trimmed();
    if (trimmed.isEmpty()) {
        trimmed = defaultRecipesUrl();
    }
    if (m_recipesUrl != trimmed) {
        m_recipesUrl = trimmed;
        saveSettings();
        emit recipesUrlChanged();
    }
}

void SettingsManager::clearAllUserData() {
    m_settings.clear();
#ifdef __EMSCRIPTEN__
    wasmLocalStorageRemove("malzeit_settings_v1");
    wasmLocalStorageRemove("malzeit_pantry_v1");
    wasmLocalStorageRemove("malzeit_pending_pantry_delta_v1");
    wasmLocalStorageRemove("malzeit_pending_starred_delta_v1");
    wasmLocalStorageRemove("malzeit_pending_store_delta_v1");
#endif
    m_starredRecipeIds.clear();
    m_syncUrl.clear();
    m_syncRoomId.clear();
    m_syncToken.clear();
    m_autoSync = true;
    m_activeStoreId = QStringLiteral("stamm-supermarkt");
    m_recipesUrl = defaultRecipesUrl();
    initializeDefaultSlots();
    saveSettings();
    emit timeSlotsChanged();
    emit syncUrlChanged();
    emit syncRoomIdChanged();
    emit syncTokenChanged();
    emit autoSyncChanged();
    emit activeStoreIdChanged();
    emit recipesUrlChanged();
    emit userDataReset();
}
