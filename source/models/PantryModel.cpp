#include "PantryModel.h"
#include <algorithm>

PantryModel::PantryModel(RecipeRepository *repo, SettingsManager *settings, QObject *parent)
    : QAbstractListModel(parent)
    , m_repo(repo)
    , m_settings(settings)
{
    if (m_settings) {
        m_availabilityMap = m_settings->loadPantryMap();
        connect(m_settings, &SettingsManager::userDataReset, this, [this]() {
            m_availabilityMap.clear();
            rebuildIngredientList();
        });
    }

    if (m_repo) {
        connect(m_repo, &RecipeRepository::recipesChanged, this, &PantryModel::rebuildIngredientList);
        connect(m_repo, &RecipeRepository::activeStoreChanged, this, &PantryModel::rebuildIngredientList);
    }

    rebuildIngredientList();
}

QString PantryModel::canonicalName(const QString &ingredientName) const {
    if (m_repo) {
        return m_repo->canonicalIngredientName(ingredientName);
    }
    return ingredientName.trimmed();
}

void PantryModel::rebuildIngredientList() {
    beginResetModel();
    m_allItems.clear();

    if (!m_repo) {
        endResetModel();
        return;
    }

    // Normalize any legacy alias keys in m_availabilityMap
    QMap<QString, bool> normalizedMap;
    for (auto it = m_availabilityMap.constBegin(); it != m_availabilityMap.constEnd(); ++it) {
        const QString canon = canonicalName(it.key());
        if (!canon.isEmpty()) {
            normalizedMap[canon] = normalizedMap.value(canon, false) || it.value();
        }
    }
    m_availabilityMap = normalizedMap;

    // Count distinct recipes per canonical ingredient
    QMap<QString, QSet<QString>> recipesByIngredient;
    for (const auto &recipe : m_repo->recipes()) {
        for (const auto &ing : recipe.ingredients()) {
            const QString canon = canonicalName(ing.name);
            if (!canon.isEmpty()) {
                recipesByIngredient[canon].insert(recipe.id());
            }
        }
    }

    QStringList names = recipesByIngredient.keys();
    std::sort(names.begin(), names.end(), [](const QString &a, const QString &b) {
        return a.localeAwareCompare(b) < 0;
    });

    for (const auto &name : names) {
        const bool avail = m_availabilityMap.value(name, false);
        PantryItem item;
        item.name = name;
        item.available = avail;
        item.recipeCount = recipesByIngredient[name].size();
        item.categoryName = m_repo->ingredientCategoryName(name);
        item.categoryOrder = m_repo->ingredientCategoryOrder(name);
        item.aliases = m_repo->aliasesForCanonical(name);
        m_allItems.append(item);
    }

    updateFilteredList();
    endResetModel();

    emit countsChanged();
    emit categoriesChanged();
    emit pantryChanged();
}

void PantryModel::updateFilteredList() {
    m_filteredItems.clear();
    const QString query = m_filterText.trimmed().toLower();
    const bool hasCatFilter = !m_categoryFilter.isEmpty() && m_categoryFilter != QLatin1String("Alle");

    for (const auto &item : m_allItems) {
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

        if (m_filterMode == 1 && !item.available) {
            continue; // Available only
        }
        if (m_filterMode == 2 && item.available) {
            continue; // Missing only
        }

        m_filteredItems.append(item);
    }

    std::sort(m_filteredItems.begin(), m_filteredItems.end(), [this](const PantryItem &a, const PantryItem &b) {
        if (m_sortOrder == 1) { // Store aisle order
            if (a.categoryOrder != b.categoryOrder) {
                return a.categoryOrder < b.categoryOrder;
            }
        }
        return a.name.localeAwareCompare(b.name) < 0;
    });
}

int PantryModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_filteredItems.size();
}

QVariant PantryModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_filteredItems.size()) {
        return QVariant();
    }

    const auto &item = m_filteredItems.at(index.row());
    switch (role) {
    case NameRole:
    case Qt::DisplayRole:
        return item.name;
    case AvailableRole:
        return item.available;
    case RecipeCountRole:
        return item.recipeCount;
    case CategoryRole:
        return item.categoryName;
    case CategoryOrderRole:
        return item.categoryOrder;
    case AliasesRole:
        return item.aliases;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PantryModel::roleNames() const {
    return {
        {NameRole, "name"},
        {AvailableRole, "available"},
        {RecipeCountRole, "recipeCount"},
        {CategoryRole, "category"},
        {CategoryOrderRole, "categoryOrder"},
        {AliasesRole, "aliases"}
    };
}

int PantryModel::availableCount() const {
    int count = 0;
    for (const auto &item : m_allItems) {
        if (item.available) {
            count++;
        }
    }
    return count;
}

QStringList PantryModel::availableCategories() const {
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

void PantryModel::setFilterText(const QString &text) {
    if (m_filterText != text) {
        beginResetModel();
        m_filterText = text;
        updateFilteredList();
        endResetModel();
        emit filterTextChanged();
    }
}

void PantryModel::setFilterMode(int mode) {
    if (m_filterMode != mode) {
        beginResetModel();
        m_filterMode = mode;
        updateFilteredList();
        endResetModel();
        emit filterModeChanged();
    }
}

void PantryModel::setCategoryFilter(const QString &category) {
    const QString target = category.isEmpty() ? QStringLiteral("Alle") : category;
    if (m_categoryFilter != target) {
        beginResetModel();
        m_categoryFilter = target;
        updateFilteredList();
        endResetModel();
        emit categoryFilterChanged();
    }
}

void PantryModel::setSortOrder(int order) {
    if (m_sortOrder != order) {
        beginResetModel();
        m_sortOrder = order;
        updateFilteredList();
        endResetModel();
        emit sortOrderChanged();
    }
}

void PantryModel::toggleAvailable(int visibleIndex) {
    if (visibleIndex < 0 || visibleIndex >= m_filteredItems.size()) return;
    const QString itemName = m_filteredItems.at(visibleIndex).name;
    const bool current = m_filteredItems.at(visibleIndex).available;
    setItemAvailable(itemName, !current);
}

void PantryModel::setItemAvailable(const QString &ingredientName, bool available) {
    const QString canon = canonicalName(ingredientName);
    if (canon.isEmpty()) return;

    m_availabilityMap[canon] = available;

    for (auto &item : m_allItems) {
        if (item.name == canon) {
            item.available = available;
            break;
        }
    }

    for (int i = 0; i < m_filteredItems.size(); ++i) {
        if (m_filteredItems[i].name == canon) {
            if ((m_filterMode == 1 && !available) || (m_filterMode == 2 && available)) {
                // Item no longer matches the active filter, remove row smoothly without jumping
                beginRemoveRows(QModelIndex(), i, i);
                m_filteredItems.removeAt(i);
                endRemoveRows();
            } else {
                m_filteredItems[i].available = available;
                const QModelIndex idx = index(i);
                emit dataChanged(idx, idx, {AvailableRole});
            }
            break;
        }
    }

    saveToSettings();
    emit countsChanged();
    emit pantryChanged();
    emit itemAvailabilityEdited(canon, available);
}

bool PantryModel::isIngredientAvailable(const QString &ingredientName) const {
    const QString canon = canonicalName(ingredientName);
    if (canon.isEmpty()) return false;
    return m_availabilityMap.value(canon, false);
}

void PantryModel::selectAll() {
    beginResetModel();
    QMap<QString, bool> changes;
    for (auto &item : m_allItems) {
        item.available = true;
        m_availabilityMap[item.name] = true;
        changes[item.name] = true;
    }
    updateFilteredList();
    endResetModel();

    saveToSettings();
    emit countsChanged();
    emit pantryChanged();
    emit bulkAvailabilityEdited(changes);
}

void PantryModel::clearAll() {
    beginResetModel();
    QMap<QString, bool> changes;
    for (auto &item : m_allItems) {
        item.available = false;
        m_availabilityMap[item.name] = false;
        changes[item.name] = false;
    }
    updateFilteredList();
    endResetModel();

    saveToSettings();
    emit countsChanged();
    emit pantryChanged();
    emit bulkAvailabilityEdited(changes);
}

QMap<QString, bool> PantryModel::getPantryMap() const {
    return m_availabilityMap;
}

void PantryModel::applyPantryMap(const QMap<QString, bool> &map) {
    QMap<QString, bool> normalizedMap;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        const QString canon = canonicalName(it.key());
        if (!canon.isEmpty()) {
            normalizedMap[canon] = normalizedMap.value(canon, false) || it.value();
        }
    }

    // Find which canonical items actually changed compared to current state
    QStringList changedNames;
    for (const auto &item : m_allItems) {
        const bool oldVal = m_availabilityMap.value(item.name, false);
        const bool newVal = normalizedMap.value(item.name, false);
        if (oldVal != newVal) {
            changedNames.append(item.name);
        }
    }

    // If nothing changed (e.g. server echoed our own delta back), do not reset or emit signals
    if (changedNames.isEmpty()) {
        return;
    }

    for (const QString &canon : changedNames) {
        const bool available = normalizedMap.value(canon, false);
        m_availabilityMap[canon] = available;

        for (auto &item : m_allItems) {
            if (item.name == canon) {
                item.available = available;
                break;
            }
        }

        int existingVisibleIdx = -1;
        for (int i = 0; i < m_filteredItems.size(); ++i) {
            if (m_filteredItems[i].name == canon) {
                existingVisibleIdx = i;
                break;
            }
        }

        if (existingVisibleIdx >= 0) {
            if ((m_filterMode == 1 && !available) || (m_filterMode == 2 && available)) {
                beginRemoveRows(QModelIndex(), existingVisibleIdx, existingVisibleIdx);
                m_filteredItems.removeAt(existingVisibleIdx);
                endRemoveRows();
            } else {
                m_filteredItems[existingVisibleIdx].available = available;
                const QModelIndex idx = index(existingVisibleIdx);
                emit dataChanged(idx, idx, {AvailableRole});
            }
        } else if ((m_filterMode == 1 && available) || (m_filterMode == 2 && !available)) {
            const QList<PantryItem> oldList = m_filteredItems;
            updateFilteredList();
            const QList<PantryItem> newList = m_filteredItems;
            m_filteredItems = oldList;
            for (int i = 0; i < newList.size(); ++i) {
                if (newList[i].name == canon) {
                    beginInsertRows(QModelIndex(), i, i);
                    m_filteredItems.insert(i, newList[i]);
                    endInsertRows();
                    break;
                }
            }
        }
    }

    saveToSettings();
    emit countsChanged();
    emit pantryChanged();
}

void PantryModel::saveToSettings() {
    if (m_settings) {
        m_settings->savePantryMap(m_availabilityMap);
    }
}
