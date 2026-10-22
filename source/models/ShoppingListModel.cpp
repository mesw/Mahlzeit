#include "ShoppingListModel.h"
#include <algorithm>

ShoppingListModel::ShoppingListModel(PantryModel *pantry, RecipeRepository *repo, SettingsManager *settings, QObject *parent)
    : QAbstractListModel(parent)
    , m_pantry(pantry)
    , m_repo(repo)
    , m_settings(settings)
{
    if (m_pantry) {
        connect(m_pantry, &PantryModel::pantryChanged, this, &ShoppingListModel::rebuildList);
    }
    if (m_repo) {
        connect(m_repo, &RecipeRepository::recipesChanged, this, &ShoppingListModel::rebuildList);
    }
    if (m_settings) {
        connect(m_settings, &SettingsManager::starredChanged, this, &ShoppingListModel::rebuildList);
    }

    rebuildList();
}

void ShoppingListModel::rebuildList() {
    if (m_skipNextRebuild) {
        return;
    }

    beginResetModel();
    m_allItems.clear();

    if (!m_repo || !m_pantry) {
        m_displayItems.clear();
        endResetModel();
        emit itemsChanged();
        return;
    }

    // Map each ingredient to the recipes that use it
    QMap<QString, QStringList> recipeUsage;
    QMap<QString, bool> starredUsage;

    for (const auto &recipe : m_repo->recipes()) {
        const bool isStarred = recipe.isStarred();
        for (const auto &ing : recipe.ingredients()) {
            const QString name = ing.name.trimmed();
            if (name.isEmpty()) continue;

            recipeUsage[name].append(recipe.dishName());
            if (isStarred) {
                starredUsage[name] = true;
            }
        }
    }

    for (auto it = recipeUsage.begin(); it != recipeUsage.end(); ++it) {
        const QString name = it.key();
        // If not in pantry, it belongs on the shopping list
        if (!m_pantry->isIngredientAvailable(name)) {
            ShoppingItem item;
            item.name = name;
            item.usedInRecipes = it.value();
            item.usedInRecipes.removeDuplicates();
            item.recipeCount = item.usedInRecipes.size();
            item.isUsedInStarredRecipe = starredUsage.value(name, false);
            m_allItems.append(item);
        }
    }

    filterAndSort();
    endResetModel();
    emit itemsChanged();
}

void ShoppingListModel::filterAndSort() {
    m_displayItems.clear();
    const QString query = m_searchQuery.trimmed().toLower();

    for (const auto &item : m_allItems) {
        if (m_onlyStarred && !item.isUsedInStarredRecipe) {
            continue;
        }
        if (!query.isEmpty() && !item.name.toLower().contains(query)) {
            continue;
        }
        m_displayItems.append(item);
    }

    // Sort
    std::sort(m_displayItems.begin(), m_displayItems.end(), [this](const ShoppingItem &a, const ShoppingItem &b) {
        if (m_sortOrder == 1) { // Starred first
            if (a.isUsedInStarredRecipe != b.isUsedInStarredRecipe) {
                return a.isUsedInStarredRecipe > b.isUsedInStarredRecipe;
            }
        } else if (m_sortOrder == 2) { // Most needed first
            if (a.recipeCount != b.recipeCount) {
                return a.recipeCount > b.recipeCount;
            }
        }
        return a.name.localeAwareCompare(b.name) < 0;
    });
}

int ShoppingListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_displayItems.size();
}

QVariant ShoppingListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_displayItems.size()) {
        return QVariant();
    }

    const auto &item = m_displayItems.at(index.row());
    switch (role) {
    case NameRole:
    case Qt::DisplayRole:
        return item.name;
    case UsedInRecipesRole:
        return item.usedInRecipes;
    case UsedInRecipesSummaryRole: {
        if (item.usedInRecipes.isEmpty()) return QString();
        if (item.usedInRecipes.size() <= 2) {
            return item.usedInRecipes.join(", ");
        }
        return QString("%1, %2 (+%3 weitere)")
            .arg(item.usedInRecipes[0])
            .arg(item.usedInRecipes[1])
            .arg(item.usedInRecipes.size() - 2);
    }
    case IsUsedInStarredRole:
        return item.isUsedInStarredRecipe;
    case RecipeCountRole:
        return item.recipeCount;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ShoppingListModel::roleNames() const {
    return {
        {NameRole, "name"},
        {UsedInRecipesRole, "usedInRecipes"},
        {UsedInRecipesSummaryRole, "usedInRecipesSummary"},
        {IsUsedInStarredRole, "isUsedInStarred"},
        {RecipeCountRole, "recipeCount"}
    };
}

void ShoppingListModel::setOnlyStarred(bool enabled) {
    if (m_onlyStarred != enabled) {
        beginResetModel();
        m_onlyStarred = enabled;
        filterAndSort();
        endResetModel();
        emit onlyStarredChanged();
        emit itemsChanged();
    }
}

void ShoppingListModel::setSearchQuery(const QString &query) {
    if (m_searchQuery != query) {
        beginResetModel();
        m_searchQuery = query;
        filterAndSort();
        endResetModel();
        emit searchQueryChanged();
        emit itemsChanged();
    }
}

void ShoppingListModel::setSortOrder(int order) {
    if (m_sortOrder != order) {
        beginResetModel();
        m_sortOrder = order;
        filterAndSort();
        endResetModel();
        emit sortOrderChanged();
    }
}

void ShoppingListModel::markPurchased(int index) {
    if (index < 0 || index >= m_displayItems.size()) return;
    markPurchasedByName(m_displayItems.at(index).name);
}

void ShoppingListModel::markPurchasedByName(const QString &name) {
    if (name.isEmpty()) return;

    // 1. Locate item in currently displayed items
    int displayIndex = -1;
    for (int i = 0; i < m_displayItems.size(); ++i) {
        if (m_displayItems[i].name == name) {
            displayIndex = i;
            break;
        }
    }

    // 2. Remove from overall list
    for (int i = 0; i < m_allItems.size(); ++i) {
        if (m_allItems[i].name == name) {
            m_allItems.removeAt(i);
            break;
        }
    }

    // 3. Smoothly remove only this row from the model without resetting scroll position
    if (displayIndex >= 0) {
        beginRemoveRows(QModelIndex(), displayIndex, displayIndex);
        m_displayItems.removeAt(displayIndex);
        endRemoveRows();
        emit itemsChanged();
    }

    // 4. Update pantry state without triggering destructive model reset
    if (m_pantry) {
        m_skipNextRebuild = true;
        m_pantry->setItemAvailable(name, true);
        m_skipNextRebuild = false;
    }
}

QString ShoppingListModel::formattedShoppingList() const {
    QString text = QStringLiteral("🛒 Einkaufsliste (Malzeit)\n");
    text += QStringLiteral("===========================\n\n");
    if (m_displayItems.isEmpty()) {
        text += QStringLiteral("Alles vorrätig! Keine offenen Einkäufe.\n");
        return text;
    }

    for (const auto &item : m_displayItems) {
        const QString star = item.isUsedInStarredRecipe ? QStringLiteral(" ★") : QString();
        text += QString("- [ ] %1%2 (für: %3)\n")
            .arg(item.name)
            .arg(star)
            .arg(item.usedInRecipes.join(", "));
    }
    return text;
}
