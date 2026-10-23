#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include "PantryModel.h"
#include "services/RecipeRepository.h"
#include "services/SettingsManager.h"

struct ShoppingItem {
    QString name;
    QStringList usedInRecipes;
    bool isUsedInStarredRecipe{false};
    int recipeCount{0};
    QString categoryName;
    int categoryOrder{999};
    QStringList aliases;
};

class ShoppingListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int itemCount READ itemCount NOTIFY itemsChanged)
    Q_PROPERTY(bool onlyStarred READ onlyStarred WRITE setOnlyStarred NOTIFY onlyStarredChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(QString categoryFilter READ categoryFilter WRITE setCategoryFilter NOTIFY categoryFilterChanged)
    Q_PROPERTY(QStringList availableCategories READ availableCategories NOTIFY categoriesChanged)
    Q_PROPERTY(int sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged) // 0: Store aisle order, 1: A-Z, 2: Starred first, 3: Most needed

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        UsedInRecipesRole,
        UsedInRecipesSummaryRole,
        IsUsedInStarredRole,
        RecipeCountRole,
        CategoryRole,
        CategoryOrderRole
    };
    Q_ENUM(Roles)

    explicit ShoppingListModel(PantryModel *pantry, RecipeRepository *repo, SettingsManager *settings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int itemCount() const { return m_displayItems.size(); }

    bool onlyStarred() const { return m_onlyStarred; }
    void setOnlyStarred(bool enabled);

    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString &query);

    QString categoryFilter() const { return m_categoryFilter; }
    void setCategoryFilter(const QString &category);

    QStringList availableCategories() const;

    int sortOrder() const { return m_sortOrder; }
    void setSortOrder(int order);

    Q_INVOKABLE void markPurchased(int index);
    Q_INVOKABLE void markPurchasedByName(const QString &name);
    Q_INVOKABLE QString formattedShoppingList() const;

signals:
    void itemsChanged();
    void onlyStarredChanged();
    void searchQueryChanged();
    void categoryFilterChanged();
    void categoriesChanged();
    void sortOrderChanged();

public slots:
    void rebuildList();

private:
    void filterAndSort();

    PantryModel *m_pantry{nullptr};
    RecipeRepository *m_repo{nullptr};
    SettingsManager *m_settings{nullptr};

    QList<ShoppingItem> m_allItems;
    QList<ShoppingItem> m_displayItems;

    bool m_onlyStarred{false};
    QString m_searchQuery;
    QString m_categoryFilter{QStringLiteral("Alle")};
    int m_sortOrder{0};
    bool m_skipNextRebuild{false};
};
