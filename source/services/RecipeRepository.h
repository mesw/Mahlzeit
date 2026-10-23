#pragma once

#include <QObject>
#include <QList>
#include <QMap>
#include <QHash>
#include <QVariantList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "models/Recipe.h"
#include "SettingsManager.h"

struct IngredientCategory {
    QString id;
    QString name;
    QStringList items;
    int defaultOrder{0};
};

struct StoreProfile {
    QString id;
    QString name;
    QStringList categoryOrder;
    QMap<QString, QString> itemOverrides;
};

class RecipeRepository : public QObject {
    Q_OBJECT
    Q_PROPERTY(int recipeCount READ recipeCount NOTIFY recipesChanged)
    Q_PROPERTY(bool isRefreshing READ isRefreshing NOTIFY isRefreshingChanged)
    Q_PROPERTY(QString lastRefreshStatus READ lastRefreshStatus NOTIFY lastRefreshStatusChanged)
    Q_PROPERTY(QVariantList stores READ storesVariantList NOTIFY recipesChanged)
    Q_PROPERTY(QString activeStoreName READ activeStoreName NOTIFY activeStoreChanged)

public:
    explicit RecipeRepository(SettingsManager *settings, QObject *parent = nullptr);

    void loadRecipes();

    QList<Recipe> recipes() const { return m_recipes; }
    int recipeCount() const { return m_recipes.size(); }
    Recipe findById(const QString &id) const;

    // Ingredient catalog, aliases, categories & store layouts
    QString canonicalIngredientName(const QString &rawName) const;
    QStringList aliasesForCanonical(const QString &canonicalName) const;
    QString ingredientCategoryId(const QString &ingredientName, const QString &storeId = QString()) const;
    QString ingredientCategoryName(const QString &ingredientName, const QString &storeId = QString()) const;
    int ingredientCategoryOrder(const QString &ingredientName, const QString &storeId = QString()) const;

    QList<StoreProfile> stores() const { return m_stores; }
    QVariantList storesVariantList() const;
    QString activeStoreName() const;
    QStringList orderedCategoryNames(const QString &storeId = QString()) const;

    Q_INVOKABLE void refreshFromWeb();
    Q_INVOKABLE void resetToBundledDefaults();
    Q_INVOKABLE void updateRecipeInstructions(const QString &recipeId, const QString &markdown);

    bool isRefreshing() const { return m_isRefreshing; }
    QString lastRefreshStatus() const { return m_lastRefreshStatus; }

signals:
    void recipesChanged();
    void activeStoreChanged();
    void isRefreshingChanged();
    void lastRefreshStatusChanged();
    void refreshFinished(bool success, const QString &message);

private:
    void loadFromResource();
    void saveToLocalCache();
    bool loadFromLocalCache();
    bool parseJsonData(const QByteArray &data,
                       QList<Recipe> &outRecipes,
                       QMap<QString, QStringList> &outAliases,
                       QList<IngredientCategory> &outCategories,
                       QList<StoreProfile> &outStores);
    void rebuildCatalogLookups();
    QString effectiveStoreId(const QString &storeId) const;

    SettingsManager *m_settings{nullptr};
    QList<Recipe> m_recipes;
    QMap<QString, QStringList> m_aliases;
    QHash<QString, QString> m_aliasToCanonicalLower;
    QList<IngredientCategory> m_categories;
    QHash<QString, IngredientCategory> m_categoryById;
    QHash<QString, QString> m_itemToCategoryIdLower;
    QList<StoreProfile> m_stores;

    QNetworkAccessManager m_networkManager;
    bool m_isRefreshing{false};
    QString m_lastRefreshStatus;
};
