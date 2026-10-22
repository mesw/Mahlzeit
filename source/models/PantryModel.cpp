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
    }

    rebuildIngredientList();
}

void PantryModel::rebuildIngredientList() {
    beginResetModel();
    m_allItems.clear();

    if (!m_repo) {
        endResetModel();
        return;
    }

    QMap<QString, int> counts;
    for (const auto &recipe : m_repo->recipes()) {
        for (const auto &ing : recipe.ingredients()) {
            if (!ing.name.trimmed().isEmpty()) {
                counts[ing.name.trimmed()]++;
            }
        }
    }

    QStringList names = counts.keys();
    std::sort(names.begin(), names.end(), [](const QString &a, const QString &b) {
        return a.localeAwareCompare(b) < 0;
    });

    for (const auto &name : names) {
        const bool avail = m_availabilityMap.value(name, false);
        m_allItems.append(PantryItem{name, avail, counts[name]});
    }

    updateFilteredList();
    endResetModel();

    emit countsChanged();
    emit pantryChanged();
}

void PantryModel::updateFilteredList() {
    m_filteredItems.clear();
    const QString query = m_filterText.trimmed().toLower();

    for (const auto &item : m_allItems) {
        if (!query.isEmpty() && !item.name.toLower().contains(query)) {
            continue;
        }

        if (m_filterMode == 1 && !item.available) {
            continue; // Available only
        }
        if (m_filterMode == 2 && item.available) {
            continue; // Missing only
        }

        m_filteredItems.append(item);
    }
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
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PantryModel::roleNames() const {
    return {
        {NameRole, "name"},
        {AvailableRole, "available"},
        {RecipeCountRole, "recipeCount"}
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

void PantryModel::toggleAvailable(int visibleIndex) {
    if (visibleIndex < 0 || visibleIndex >= m_filteredItems.size()) return;
    const QString itemName = m_filteredItems.at(visibleIndex).name;
    const bool current = m_filteredItems.at(visibleIndex).available;
    setItemAvailable(itemName, !current);
}

void PantryModel::setItemAvailable(const QString &ingredientName, bool available) {
    if (ingredientName.isEmpty()) return;

    m_availabilityMap[ingredientName] = available;

    for (auto &item : m_allItems) {
        if (item.name == ingredientName) {
            item.available = available;
            break;
        }
    }

    for (int i = 0; i < m_filteredItems.size(); ++i) {
        if (m_filteredItems[i].name == ingredientName) {
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
}

bool PantryModel::isIngredientAvailable(const QString &ingredientName) const {
    return m_availabilityMap.value(ingredientName, false);
}

void PantryModel::selectAll() {
    beginResetModel();
    for (auto &item : m_allItems) {
        item.available = true;
        m_availabilityMap[item.name] = true;
    }
    updateFilteredList();
    endResetModel();

    saveToSettings();
    emit countsChanged();
    emit pantryChanged();
}

void PantryModel::clearAll() {
    beginResetModel();
    for (auto &item : m_allItems) {
        item.available = false;
        m_availabilityMap[item.name] = false;
    }
    updateFilteredList();
    endResetModel();

    saveToSettings();
    emit countsChanged();
    emit pantryChanged();
}

QMap<QString, bool> PantryModel::getPantryMap() const {
    return m_availabilityMap;
}

void PantryModel::applyPantryMap(const QMap<QString, bool> &map) {
    beginResetModel();
    m_availabilityMap = map;
    for (auto &item : m_allItems) {
        item.available = m_availabilityMap.value(item.name, false);
    }
    updateFilteredList();
    endResetModel();

    saveToSettings();
    emit countsChanged();
    emit pantryChanged();
}

void PantryModel::saveToSettings() {
    if (m_settings) {
        m_settings->savePantryMap(m_availabilityMap);
    }
}
