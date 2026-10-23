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
        connect(m_repo, &RecipeRepository::activeStoreChanged, this, &ShoppingListModel::rebuildList);
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

    const QStringList oldCategories = availableCategories();
    const QList<ShoppingItem> oldDisplayItems = m_displayItems;

    m_allItems.clear();

    if (!m_repo || !m_pantry) {
        if (!m_displayItems.isEmpty()) {
            beginRemoveRows(QModelIndex(), 0, m_displayItems.size() - 1);
            m_displayItems.clear();
            endRemoveRows();
        }
        emit itemsChanged();
        if (availableCategories() != oldCategories) {
            emit categoriesChanged();
        }
        return;
    }

    // Map each canonical ingredient to the recipes that use it
    QMap<QString, QStringList> recipeUsage;
    QMap<QString, bool> starredUsage;

    for (const auto &recipe : m_repo->recipes()) {
        const bool isStarred = recipe.isStarred();
        for (const auto &ing : recipe.ingredients()) {
            const QString canon = m_repo->canonicalIngredientName(ing.name);
            if (canon.isEmpty()) continue;

            recipeUsage[canon].append(recipe.dishName());
            if (isStarred) {
                starredUsage[canon] = true;
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
            item.categoryName = m_repo->ingredientCategoryName(name);
            item.categoryOrder = m_repo->ingredientCategoryOrder(name);
            item.aliases = m_repo->aliasesForCanonical(name);
            m_allItems.append(item);
        }
    }

    filterAndSort(); // populates m_displayItems with the new target state
    const QList<ShoppingItem> newDisplayItems = m_displayItems;
    m_displayItems = oldDisplayItems;

    // 1. Remove rows that are no longer in newDisplayItems
    QSet<QString> newNames;
    for (const auto &item : newDisplayItems) {
        newNames.insert(item.name);
    }
    for (int i = m_displayItems.size() - 1; i >= 0; --i) {
        if (!newNames.contains(m_displayItems.at(i).name)) {
            beginRemoveRows(QModelIndex(), i, i);
            m_displayItems.removeAt(i);
            endRemoveRows();
        }
    }

    // 2. Move or insert rows to match newDisplayItems order without resetting scroll position
    for (int i = 0; i < newDisplayItems.size(); ++i) {
        const QString &targetName = newDisplayItems.at(i).name;
        if (i < m_displayItems.size() && m_displayItems.at(i).name == targetName) {
            continue;
        }

        int from = -1;
        for (int j = i + 1; j < m_displayItems.size(); ++j) {
            if (m_displayItems.at(j).name == targetName) {
                from = j;
                break;
            }
        }

        if (from >= 0) {
            beginMoveRows(QModelIndex(), from, from, QModelIndex(), i);
            m_displayItems.move(from, i);
            endMoveRows();
        } else {
            beginInsertRows(QModelIndex(), i, i);
            m_displayItems.insert(i, newDisplayItems.at(i));
            endInsertRows();
        }
    }

    // 3. Update item data in place
    m_displayItems = newDisplayItems;
    if (!m_displayItems.isEmpty()) {
        emit dataChanged(index(0), index(m_displayItems.size() - 1));
    }

    emit itemsChanged();
    if (availableCategories() != oldCategories) {
        emit categoriesChanged();
    }
}

void ShoppingListModel::filterAndSort() {
    m_displayItems.clear();
    const QString query = m_searchQuery.trimmed().toLower();
    const bool hasCatFilter = !m_categoryFilter.isEmpty() && m_categoryFilter != QLatin1String("Alle");

    for (const auto &item : m_allItems) {
        if (m_onlyStarred && !item.isUsedInStarredRecipe) {
            continue;
        }
        if (hasCatFilter && item.categoryName.compare(m_categoryFilter, Qt::CaseInsensitive) != 0) {
            continue;
        }
        if (!query.isEmpty()) {
            bool matches = item.name.toLower().contains(query) ||
                           item.categoryName.toLower().contains(query);
            if (!matches) {
                for (const QString &alias : item.aliases) {
                    if (alias.toLower().contains(query)) {
                        matches = true;
                        break;
                    }
                }
            }
            if (!matches) {
                continue;
            }
        }
        m_displayItems.append(item);
    }

    // Sort
    std::sort(m_displayItems.begin(), m_displayItems.end(), [this](const ShoppingItem &a, const ShoppingItem &b) {
        if (m_sortOrder == 0) { // Store aisle order
            if (a.categoryOrder != b.categoryOrder) {
                return a.categoryOrder < b.categoryOrder;
            }
        } else if (m_sortOrder == 2) { // Starred first
            if (a.isUsedInStarredRecipe != b.isUsedInStarredRecipe) {
                return a.isUsedInStarredRecipe > b.isUsedInStarredRecipe;
            }
        } else if (m_sortOrder == 3) { // Most needed first
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
    case CategoryRole:
        return item.categoryName;
    case CategoryOrderRole:
        return item.categoryOrder;
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
        {RecipeCountRole, "recipeCount"},
        {CategoryRole, "category"},
        {CategoryOrderRole, "categoryOrder"}
    };
}

QStringList ShoppingListModel::availableCategories() const {
    QStringList result;
    result.append(QStringLiteral("Alle"));
    if (!m_repo) return result;

    QSet<QString> presentCategories;
    for (const auto &item : m_allItems) {
        if (!item.categoryName.isEmpty()) {
            presentCategories.insert(item.categoryName);
        }
    }

    for (const QString &orderedCat : m_repo->orderedCategoryNames()) {
        if (presentCategories.contains(orderedCat) && !result.contains(orderedCat)) {
            result.append(orderedCat);
        }
    }

    for (const QString &cat : presentCategories) {
        if (!result.contains(cat)) {
            result.append(cat);
        }
    }
    return result;
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

void ShoppingListModel::setCategoryFilter(const QString &category) {
    const QString target = category.isEmpty() ? QStringLiteral("Alle") : category;
    if (m_categoryFilter != target) {
        beginResetModel();
        m_categoryFilter = target;
        filterAndSort();
        endResetModel();
        emit categoryFilterChanged();
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
    const QString canon = m_repo ? m_repo->canonicalIngredientName(name) : name.trimmed();
    if (canon.isEmpty()) return;

    // 1. Locate item in currently displayed items
    int displayIndex = -1;
    for (int i = 0; i < m_displayItems.size(); ++i) {
        if (m_displayItems[i].name == canon) {
            displayIndex = i;
            break;
        }
    }

    // 2. Remove from overall list
    for (int i = 0; i < m_allItems.size(); ++i) {
        if (m_allItems[i].name == canon) {
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
        m_pantry->setItemAvailable(canon, true);
        m_skipNextRebuild = false;
    }
}

QString ShoppingListModel::formattedShoppingList() const {
    const QString storeName = m_repo ? m_repo->activeStoreName() : QString();
    QString text = storeName.isEmpty()
        ? QStringLiteral("🛒 Einkaufsliste (Mahlzeit)\n")
        : QString("🛒 Einkaufsliste – %1 (Mahlzeit)\n").arg(storeName);
    text += QStringLiteral("===========================\n\n");
    if (m_displayItems.isEmpty()) {
        text += QStringLiteral("Alles vorrätig! Keine offenen Einkäufe.\n");
        return text;
    }

    QString lastCategory;
    for (const auto &item : m_displayItems) {
        if (m_sortOrder == 0 && item.categoryName != lastCategory) {
            if (!lastCategory.isEmpty()) {
                text += QStringLiteral("\n");
            }
            text += QString("▸ %1\n").arg(item.categoryName);
            lastCategory = item.categoryName;
        }
        const QString star = item.isUsedInStarredRecipe ? QStringLiteral(" ★") : QString();
        text += QString("- [ ] %1%2 (für: %3)\n")
            .arg(item.name)
            .arg(star)
            .arg(item.usedInRecipes.join(", "));
    }
    return text;
}
