#include "RecipeRepository.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QUrl>
#include <QNetworkRequest>

RecipeRepository::RecipeRepository(SettingsManager *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    loadRecipes();

    if (m_settings) {
        connect(m_settings, &SettingsManager::starredChanged, this, [this](const QString &id, bool starred) {
            for (auto &r : m_recipes) {
                if (r.id() == id) {
                    r.setStarred(starred);
                    break;
                }
            }
            emit recipesChanged();
        });

        connect(m_settings, &SettingsManager::activeStoreIdChanged, this, [this]() {
            emit activeStoreChanged();
        });

        connect(m_settings, &SettingsManager::userDataReset, this, [this]() {
            resetToBundledDefaults();
        });
    }
}

void RecipeRepository::loadRecipes() {
    // 1. Try to load from user settings cache first
    if (!loadFromLocalCache()) {
        // 2. First run or outdated cache: load from bundled resource and save to user settings
        loadFromResource();
        saveToLocalCache();
    }

    // Apply starred states
    if (m_settings) {
        for (auto &r : m_recipes) {
            r.setStarred(m_settings->isRecipeStarred(r.id()));
        }
    }

    emit recipesChanged();
    emit activeStoreChanged();
}

Recipe RecipeRepository::findById(const QString &id) const {
    for (const auto &r : m_recipes) {
        if (r.id() == id) {
            return r;
        }
    }
    return Recipe();
}

bool RecipeRepository::parseJsonData(const QByteArray &data,
                                     QList<Recipe> &outRecipes,
                                     QMap<QString, QStringList> &outAliases,
                                     QList<IngredientCategory> &outCategories,
                                     QList<StoreProfile> &outStores) {
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || (!doc.isObject() && !doc.isArray())) {
        qWarning() << "Failed to parse recipes JSON:" << err.errorString();
        return false;
    }

    outRecipes.clear();
    outAliases.clear();
    outCategories.clear();
    outStores.clear();

    QJsonArray recipesArr;
    if (doc.isArray()) {
        recipesArr = doc.array();
    } else {
        const QJsonObject root = doc.object();
        recipesArr = root.value("recipes").toArray();

        // Parse aliases
        if (root.value("aliases").isObject()) {
            const QJsonObject aliasesObj = root.value("aliases").toObject();
            for (auto it = aliasesObj.constBegin(); it != aliasesObj.constEnd(); ++it) {
                const QString canonical = it.key().trimmed();
                if (canonical.isEmpty() || !it.value().isArray()) continue;
                QStringList list;
                const QJsonArray arr = it.value().toArray();
                for (const auto &v : arr) {
                    const QString a = v.toString().trimmed();
                    if (!a.isEmpty()) list.append(a);
                }
                outAliases.insert(canonical, list);
            }
        }

        // Parse categories
        if (root.value("categories").isObject()) {
            const QJsonObject catsObj = root.value("categories").toObject();
            int idx = 0;
            for (auto it = catsObj.constBegin(); it != catsObj.constEnd(); ++it) {
                if (!it.value().isObject()) continue;
                const QJsonObject cObj = it.value().toObject();
                IngredientCategory cat;
                cat.id = it.key().trimmed();
                cat.name = cObj.value("name").toString().trimmed();
                if (cat.name.isEmpty()) cat.name = cat.id;
                cat.defaultOrder = idx++;
                if (cObj.value("items").isArray()) {
                    const QJsonArray itemsArr = cObj.value("items").toArray();
                    for (const auto &v : itemsArr) {
                        const QString item = v.toString().trimmed();
                        if (!item.isEmpty()) cat.items.append(item);
                    }
                }
                outCategories.append(cat);
            }
        }

        // Parse stores
        if (root.value("stores").isArray()) {
            const QJsonArray storesArr = root.value("stores").toArray();
            for (const auto &val : storesArr) {
                if (!val.isObject()) continue;
                const QJsonObject sObj = val.toObject();
                StoreProfile store;
                store.id = sObj.value("id").toString().trimmed();
                store.name = sObj.value("name").toString().trimmed();
                if (store.id.isEmpty() || store.name.isEmpty()) continue;

                const QJsonArray orderArr = sObj.value("category_order").toArray();
                for (const auto &v : orderArr) {
                    const QString cId = v.toString().trimmed();
                    if (!cId.isEmpty()) store.categoryOrder.append(cId);
                }

                if (sObj.value("item_overrides").isObject()) {
                    const QJsonObject ovObj = sObj.value("item_overrides").toObject();
                    for (auto it = ovObj.constBegin(); it != ovObj.constEnd(); ++it) {
                        store.itemOverrides.insert(it.key().trimmed(), it.value().toString().trimmed());
                    }
                }
                outStores.append(store);
            }
        }
    }

    for (const auto &val : recipesArr) {
        if (val.isObject()) {
            Recipe r = Recipe::fromJsonObject(val.toObject());
            if (!r.dishName().isEmpty()) {
                outRecipes.append(r);
            }
        }
    }
    return !outRecipes.isEmpty();
}

void RecipeRepository::rebuildCatalogLookups() {
    m_aliasToCanonicalLower.clear();
    for (auto it = m_aliases.constBegin(); it != m_aliases.constEnd(); ++it) {
        const QString canonical = it.key().trimmed();
        m_aliasToCanonicalLower.insert(canonical.toLower(), canonical);
        for (const QString &alias : it.value()) {
            m_aliasToCanonicalLower.insert(alias.trimmed().toLower(), canonical);
        }
    }

    m_categoryById.clear();
    m_itemToCategoryIdLower.clear();
    for (const auto &cat : m_categories) {
        m_categoryById.insert(cat.id, cat);
        for (const QString &item : cat.items) {
            const QString canonical = canonicalIngredientName(item);
            m_itemToCategoryIdLower.insert(canonical.toLower(), cat.id);
        }
    }
}

QString RecipeRepository::canonicalIngredientName(const QString &rawName) const {
    const QString trimmed = rawName.trimmed();
    if (trimmed.isEmpty()) return QString();
    const QString lower = trimmed.toLower();
    if (m_aliasToCanonicalLower.contains(lower)) {
        return m_aliasToCanonicalLower.value(lower);
    }
    return trimmed;
}

QStringList RecipeRepository::aliasesForCanonical(const QString &canonicalName) const {
    return m_aliases.value(canonicalName.trimmed());
}

QString RecipeRepository::effectiveStoreId(const QString &storeId) const {
    QString target = storeId.trimmed();
    if (target.isEmpty() && m_settings) {
        target = m_settings->activeStoreId();
    }
    for (const auto &s : m_stores) {
        if (s.id == target) {
            return s.id;
        }
    }
    return m_stores.isEmpty() ? QString() : m_stores.first().id;
}

QString RecipeRepository::ingredientCategoryId(const QString &ingredientName, const QString &storeId) const {
    const QString canonical = canonicalIngredientName(ingredientName);
    if (canonical.isEmpty()) return QStringLiteral("uncategorized");

    const QString activeId = effectiveStoreId(storeId);
    for (const auto &store : m_stores) {
        if (store.id == activeId) {
            for (auto it = store.itemOverrides.constBegin(); it != store.itemOverrides.constEnd(); ++it) {
                if (canonicalIngredientName(it.key()).compare(canonical, Qt::CaseInsensitive) == 0) {
                    const QString overrideCatId = it.value();
                    if (m_categoryById.contains(overrideCatId)) {
                        return overrideCatId;
                    }
                }
            }
            break;
        }
    }

    const QString catId = m_itemToCategoryIdLower.value(canonical.toLower());
    if (!catId.isEmpty() && m_categoryById.contains(catId)) {
        return catId;
    }
    return QStringLiteral("uncategorized");
}

QString RecipeRepository::ingredientCategoryName(const QString &ingredientName, const QString &storeId) const {
    const QString catId = ingredientCategoryId(ingredientName, storeId);
    if (m_categoryById.contains(catId)) {
        return m_categoryById.value(catId).name;
    }
    return tr("Sonstiges");
}

int RecipeRepository::ingredientCategoryOrder(const QString &ingredientName, const QString &storeId) const {
    const QString catId = ingredientCategoryId(ingredientName, storeId);
    if (catId == QLatin1String("uncategorized")) {
        return 999;
    }

    const QString activeId = effectiveStoreId(storeId);
    for (const auto &store : m_stores) {
        if (store.id == activeId) {
            const int idx = store.categoryOrder.indexOf(catId);
            if (idx >= 0) {
                return idx;
            }
            break;
        }
    }

    if (m_categoryById.contains(catId)) {
        return 100 + m_categoryById.value(catId).defaultOrder;
    }
    return 999;
}

QVariantList RecipeRepository::storesVariantList() const {
    QVariantList list;
    for (const auto &s : m_stores) {
        list.append(QVariantMap{
            {"id", s.id},
            {"name", s.name}
        });
    }
    return list;
}

QString RecipeRepository::activeStoreName() const {
    const QString activeId = effectiveStoreId(QString());
    for (const auto &s : m_stores) {
        if (s.id == activeId) {
            return s.name;
        }
    }
    return QString();
}

QStringList RecipeRepository::orderedCategoryNames(const QString &storeId) const {
    QStringList names;
    const QString activeId = effectiveStoreId(storeId);

    for (const auto &store : m_stores) {
        if (store.id == activeId) {
            for (const QString &catId : store.categoryOrder) {
                if (m_categoryById.contains(catId)) {
                    const QString name = m_categoryById.value(catId).name;
                    if (!names.contains(name)) {
                        names.append(name);
                    }
                }
            }
            break;
        }
    }

    for (const auto &cat : m_categories) {
        if (!names.contains(cat.name)) {
            names.append(cat.name);
        }
    }
    return names;
}

void RecipeRepository::loadFromResource() {
    const QStringList candidatePaths = {
        ":/resources/recipes.json",
        ":/recipes.json",
        ":/qt/qml/Malzeit/resources/recipes.json",
        ":/qt/qml/Malzeit/recipes.json",
        "resources/recipes.json",
        "../resources/recipes.json"
    };

    QByteArray data;
    for (const auto &path : candidatePaths) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            data = file.readAll();
            file.close();
            if (!data.isEmpty()) {
                qDebug() << "Loaded bundled recipes from:" << path;
                break;
            }
        }
    }

    if (data.isEmpty()) {
        qWarning() << "Could not open bundled recipes.json from any known path!";
        return;
    }

    QList<Recipe> loaded;
    QMap<QString, QStringList> aliases;
    QList<IngredientCategory> categories;
    QList<StoreProfile> stores;
    if (parseJsonData(data, loaded, aliases, categories, stores)) {
        m_recipes = loaded;
        m_aliases = aliases;
        m_categories = categories;
        m_stores = stores;
        rebuildCatalogLookups();
    }
}

bool RecipeRepository::loadFromLocalCache() {
    QSettings settings("Malzeit", "Malzeit");
    const QString json = settings.value("cachedRecipesJson").toString();
    if (json.isEmpty()) {
        return false;
    }
    // Invalidate stale cache if it still uses legacy "ingredients" map or legacy array-only format without categories
    if (json.contains("\"ingredients\":") || !json.trimmed().startsWith('{')) {
        return false;
    }

    QList<Recipe> loaded;
    QMap<QString, QStringList> aliases;
    QList<IngredientCategory> categories;
    QList<StoreProfile> stores;
    if (parseJsonData(json.toUtf8(), loaded, aliases, categories, stores)) {
        m_recipes = loaded;
        m_aliases = aliases;
        m_categories = categories;
        m_stores = stores;
        rebuildCatalogLookups();
        return true;
    }
    return false;
}

void RecipeRepository::saveToLocalCache() {
    QJsonObject root;

    // Aliases
    QJsonObject aliasesObj;
    for (auto it = m_aliases.constBegin(); it != m_aliases.constEnd(); ++it) {
        QJsonArray arr;
        for (const QString &a : it.value()) {
            arr.append(a);
        }
        aliasesObj.insert(it.key(), arr);
    }
    root.insert("aliases", aliasesObj);

    // Categories
    QJsonObject catsObj;
    for (const auto &cat : m_categories) {
        QJsonObject cObj;
        cObj.insert("name", cat.name);
        QJsonArray itemsArr;
        for (const QString &item : cat.items) {
            itemsArr.append(item);
        }
        cObj.insert("items", itemsArr);
        catsObj.insert(cat.id, cObj);
    }
    root.insert("categories", catsObj);

    // Stores
    QJsonArray storesArr;
    for (const auto &store : m_stores) {
        QJsonObject sObj;
        sObj.insert("id", store.id);
        sObj.insert("name", store.name);
        QJsonArray orderArr;
        for (const QString &cId : store.categoryOrder) {
            orderArr.append(cId);
        }
        sObj.insert("category_order", orderArr);
        QJsonObject ovObj;
        for (auto it = store.itemOverrides.constBegin(); it != store.itemOverrides.constEnd(); ++it) {
            ovObj.insert(it.key(), it.value());
        }
        sObj.insert("item_overrides", ovObj);
        storesArr.append(sObj);
    }
    root.insert("stores", storesArr);

    // Recipes
    QJsonArray recipesArr;
    for (const auto &r : m_recipes) {
        recipesArr.append(r.toJsonObject());
    }
    root.insert("recipes", recipesArr);

    const QJsonDocument doc(root);
    QSettings settings("Malzeit", "Malzeit");
    settings.setValue("cachedRecipesJson", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    settings.sync();
}

void RecipeRepository::resetToBundledDefaults() {
    loadFromResource();
    saveToLocalCache();
    if (m_settings) {
        for (auto &r : m_recipes) {
            r.setStarred(m_settings->isRecipeStarred(r.id()));
        }
    }
    m_lastRefreshStatus = tr("Rezepte auf Standard zurückgesetzt.");
    emit lastRefreshStatusChanged();
    emit recipesChanged();
    emit activeStoreChanged();
    emit refreshFinished(true, m_lastRefreshStatus);
}

void RecipeRepository::updateRecipeInstructions(const QString &recipeId, const QString &markdown) {
    for (auto &r : m_recipes) {
        if (r.id() == recipeId) {
            r.setInstructions(markdown);
            break;
        }
    }
    saveToLocalCache();
    emit recipesChanged();
}

void RecipeRepository::refreshFromWeb() {
    if (m_isRefreshing) return;

    m_isRefreshing = true;
    emit isRefreshingChanged();
    m_lastRefreshStatus = tr("Lade Rezepte von Server...");
    emit lastRefreshStatusChanged();

    const QString targetUrl = m_settings ? m_settings->recipesUrl() : SettingsManager::defaultRecipesUrl();
    const QUrl requestUrl(targetUrl);
    QNetworkRequest request(requestUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_isRefreshing = false;
        emit isRefreshingChanged();

        if (reply->error() != QNetworkReply::NoError) {
            m_lastRefreshStatus = tr("Fehler beim Laden: %1").arg(reply->errorString());
            emit lastRefreshStatusChanged();
            emit refreshFinished(false, m_lastRefreshStatus);
            return;
        }

        const QByteArray responseData = reply->readAll();
        QList<Recipe> newRecipes;
        QMap<QString, QStringList> newAliases;
        QList<IngredientCategory> newCategories;
        QList<StoreProfile> newStores;
        if (!parseJsonData(responseData, newRecipes, newAliases, newCategories, newStores)) {
            m_lastRefreshStatus = tr("Ungültiges JSON-Format von Server empfangen.");
            emit lastRefreshStatusChanged();
            emit refreshFinished(false, m_lastRefreshStatus);
            return;
        }

        // Preserve local instructions and stars
        for (auto &nr : newRecipes) {
            const Recipe old = findById(nr.id());
            if (!old.dishName().isEmpty()) {
                if (nr.instructions().isEmpty() && !old.instructions().isEmpty()) {
                    nr.setInstructions(old.instructions());
                }
            }
            if (m_settings) {
                nr.setStarred(m_settings->isRecipeStarred(nr.id()));
            }
        }

        m_recipes = newRecipes;
        if (!newAliases.isEmpty() || !newCategories.isEmpty() || !newStores.isEmpty()) {
            m_aliases = newAliases;
            m_categories = newCategories;
            m_stores = newStores;
            rebuildCatalogLookups();
        }
        saveToLocalCache();

        m_lastRefreshStatus = tr("%1 Rezepte erfolgreich aktualisiert!").arg(m_recipes.size());
        emit lastRefreshStatusChanged();
        emit recipesChanged();
        emit activeStoreChanged();
        emit refreshFinished(true, m_lastRefreshStatus);
    });
}
