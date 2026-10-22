#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QVariantList>
#include "Recipe.h"
#include "PantryModel.h"
#include "services/RecipeRepository.h"
#include "services/SettingsManager.h"
#include "services/SuggestionEngine.h"

struct RecipeViewModelItem {
    Recipe recipe;
    RecipeEvaluation evaluation;
};

class RecipeModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool onlyReadyToCook READ onlyReadyToCook WRITE setOnlyReadyToCook NOTIFY onlyReadyToCookChanged)
    Q_PROPERTY(bool onlyTimeMatch READ onlyTimeMatch WRITE setOnlyTimeMatch NOTIFY onlyTimeMatchChanged)
    Q_PROPERTY(bool onlyStarred READ onlyStarred WRITE setOnlyStarred NOTIFY onlyStarredChanged)
    Q_PROPERTY(QString categoryFilter READ categoryFilter WRITE setCategoryFilter NOTIFY categoryFilterChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(int sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(QStringList availableCategories READ availableCategories NOTIFY availableCategoriesChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        DishNameRole,
        CategoryRole,
        ServingsRole,
        InstructionsRole,
        IsStarredRole,
        CanMakeRole,
        MissingCountRole,
        MissingIngredientsRole,
        MissingSummaryRole,
        IsTimeMatchRole,
        SuggestionScoreRole,
        IngredientsListRole
    };
    Q_ENUM(Roles)

    explicit RecipeModel(RecipeRepository *repo, PantryModel *pantry, SettingsManager *settings, SuggestionEngine *engine, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_visibleItems.size(); }

    bool onlyReadyToCook() const { return m_onlyReadyToCook; }
    void setOnlyReadyToCook(bool enabled);

    bool onlyTimeMatch() const { return m_onlyTimeMatch; }
    void setOnlyTimeMatch(bool enabled);

    bool onlyStarred() const { return m_onlyStarred; }
    void setOnlyStarred(bool enabled);

    QString categoryFilter() const { return m_categoryFilter; }
    void setCategoryFilter(const QString &cat);

    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString &query);

    int sortMode() const { return m_sortMode; }
    void setSortMode(int mode);

    QStringList availableCategories() const;

    Q_INVOKABLE void toggleStarred(const QString &recipeId);
    Q_INVOKABLE QVariantMap getRecipe(int index) const;
    Q_INVOKABLE QVariantMap getRecipeById(const QString &id) const;
    Q_INVOKABLE QVariantList getScaledIngredients(const QString &id, int targetServings) const;

signals:
    void countChanged();
    void onlyReadyToCookChanged();
    void onlyTimeMatchChanged();
    void onlyStarredChanged();
    void categoryFilterChanged();
    void searchQueryChanged();
    void sortModeChanged();
    void availableCategoriesChanged();

public slots:
    void updateList();

private:
    void filterAndSort();

    RecipeRepository *m_repo{nullptr};
    PantryModel *m_pantry{nullptr};
    SettingsManager *m_settings{nullptr};
    SuggestionEngine *m_engine{nullptr};

    QList<RecipeViewModelItem> m_allItems;
    QList<RecipeViewModelItem> m_visibleItems;

    bool m_onlyReadyToCook{true}; // User requirement: by default shows only recipes for which all ingredients are available
    bool m_onlyTimeMatch{true};   // User requirement: and that are suitable for current time of day
    bool m_onlyStarred{false};
    QString m_categoryFilter;
    QString m_searchQuery;
    int m_sortMode{0}; // 0: Suggestion Score, 1: Name, 2: Category, 3: Missing count
};
