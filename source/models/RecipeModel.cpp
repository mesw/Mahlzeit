#include "RecipeModel.h"
#include <algorithm>
#include <QSet>

RecipeModel::RecipeModel(RecipeRepository *repo, PantryModel *pantry, SettingsManager *settings, SuggestionEngine *engine, QObject *parent)
    : QAbstractListModel(parent)
    , m_repo(repo)
    , m_pantry(pantry)
    , m_settings(settings)
    , m_engine(engine)
{
    if (m_repo) {
        connect(m_repo, &RecipeRepository::recipesChanged, this, &RecipeModel::updateList);
    }
    if (m_pantry) {
        connect(m_pantry, &PantryModel::pantryChanged, this, &RecipeModel::updateList);
    }
    if (m_engine) {
        connect(m_engine, &SuggestionEngine::evaluationChanged, this, &RecipeModel::updateList);
    }
    if (m_settings) {
        connect(m_settings, &SettingsManager::starredChanged, this, &RecipeModel::updateList);
    }

    updateList();
}

void RecipeModel::updateList() {
    m_allItems.clear();

    if (m_repo) {
        for (const auto &r : m_repo->recipes()) {
            RecipeEvaluation eval;
            if (m_engine) {
                eval = m_engine->evaluateRecipe(r);
            }
            m_allItems.append(RecipeViewModelItem{r, eval});
        }
    }

    applyFilterAndSort();
    emit countChanged();
    emit availableCategoriesChanged();
}

void RecipeModel::applyFilterAndSort() {
    const QList<RecipeViewModelItem> oldItems = m_visibleItems;
    filterAndSort(); // fills m_visibleItems with the new state
    const QList<RecipeViewModelItem> newItems = m_visibleItems;
    m_visibleItems = oldItems; // step from old to new state via incremental signals

    auto idAt = [](const QList<RecipeViewModelItem> &list, int i) { return list.at(i).recipe.id(); };

    // 1. Remove rows that no longer exist
    QSet<QString> newIds;
    for (const auto &it : newItems) {
        newIds.insert(it.recipe.id());
    }
    for (int i = m_visibleItems.size() - 1; i >= 0; --i) {
        if (!newIds.contains(idAt(m_visibleItems, i))) {
            beginRemoveRows(QModelIndex(), i, i);
            m_visibleItems.removeAt(i);
            endRemoveRows();
        }
    }

    // 2. Move / insert rows so the order matches the new state
    for (int i = 0; i < newItems.size(); ++i) {
        const QString id = idAt(newItems, i);
        if (i < m_visibleItems.size() && idAt(m_visibleItems, i) == id) {
            continue;
        }

        int from = -1;
        for (int j = i + 1; j < m_visibleItems.size(); ++j) {
            if (idAt(m_visibleItems, j) == id) {
                from = j;
                break;
            }
        }

        if (from >= 0) {
            beginMoveRows(QModelIndex(), from, from, QModelIndex(), i);
            m_visibleItems.move(from, i);
            endMoveRows();
        } else {
            beginInsertRows(QModelIndex(), i, i);
            m_visibleItems.insert(i, newItems.at(i));
            endInsertRows();
        }
    }

    // 3. Refresh data of all rows (starred state, evaluation, ...)
    m_visibleItems = newItems;
    if (!m_visibleItems.isEmpty()) {
        emit dataChanged(index(0), index(m_visibleItems.size() - 1));
    }
}

void RecipeModel::filterAndSort() {
    m_visibleItems.clear();
    const QString query = m_searchQuery.trimmed().toLower();

    for (const auto &item : m_allItems) {
        // Filter: Ready to cook
        if (m_onlyReadyToCook && !item.evaluation.canMake) {
            continue;
        }

        // Filter: Suitable for current time of day
        if (m_onlyTimeMatch && !item.evaluation.isTimeMatch) {
            continue;
        }

        // Filter: Starred only
        if (m_onlyStarred && !item.recipe.isStarred()) {
            continue;
        }

        // Filter: Category
        if (!m_categoryFilter.isEmpty() && m_categoryFilter != "Alle" &&
            item.recipe.category().compare(m_categoryFilter, Qt::CaseInsensitive) != 0) {
            continue;
        }

        // Filter: Search query (matches recipe title or any ingredient name)
        if (!query.isEmpty()) {
            bool matches = item.recipe.dishName().toLower().contains(query);
            if (!matches) {
                for (const auto &ing : item.recipe.ingredients()) {
                    if (ing.name.toLower().contains(query)) {
                        matches = true;
                        break;
                    }
                }
            }
            if (!matches) {
                continue;
            }
        }

        m_visibleItems.append(item);
    }

    // Sort
    std::sort(m_visibleItems.begin(), m_visibleItems.end(), [this](const RecipeViewModelItem &a, const RecipeViewModelItem &b) {
        if (m_sortMode == 0) { // Suggestion score descending
            if (a.evaluation.suggestionScore != b.evaluation.suggestionScore) {
                return a.evaluation.suggestionScore > b.evaluation.suggestionScore;
            }
        } else if (m_sortMode == 1) { // Alphabetical
            return a.recipe.dishName().localeAwareCompare(b.recipe.dishName()) < 0;
        } else if (m_sortMode == 2) { // Category
            int catCmp = a.recipe.category().localeAwareCompare(b.recipe.category());
            if (catCmp != 0) return catCmp < 0;
            return a.recipe.dishName().localeAwareCompare(b.recipe.dishName()) < 0;
        } else if (m_sortMode == 3) { // Missing ingredients count ascending
            if (a.evaluation.missingIngredientsCount != b.evaluation.missingIngredientsCount) {
                return a.evaluation.missingIngredientsCount < b.evaluation.missingIngredientsCount;
            }
        }
        return a.recipe.dishName().localeAwareCompare(b.recipe.dishName()) < 0;
    });
}

int RecipeModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_visibleItems.size();
}

QVariant RecipeModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibleItems.size()) {
        return QVariant();
    }

    const auto &item = m_visibleItems.at(index.row());
    switch (role) {
    case IdRole:
        return item.recipe.id();
    case DishNameRole:
    case Qt::DisplayRole:
        return item.recipe.dishName();
    case CategoryRole:
        return item.recipe.category();
    case ServingsRole:
        return item.recipe.servings();
    case InstructionsRole:
        return item.recipe.instructions();
    case IsStarredRole:
        return item.recipe.isStarred();
    case CanMakeRole:
        return item.evaluation.canMake;
    case MissingCountRole:
        return item.evaluation.missingIngredientsCount;
    case MissingIngredientsRole:
        return item.evaluation.missingIngredients;
    case MissingSummaryRole: {
        if (item.evaluation.missingIngredients.isEmpty()) {
            return QString();
        }
        if (item.evaluation.missingIngredients.size() <= 2) {
            return item.evaluation.missingIngredients.join(", ");
        }
        return QString("%1, %2 (+%3)")
            .arg(item.evaluation.missingIngredients[0])
            .arg(item.evaluation.missingIngredients[1])
            .arg(item.evaluation.missingIngredients.size() - 2);
    }
    case IsTimeMatchRole:
        return item.evaluation.isTimeMatch;
    case SuggestionScoreRole:
        return item.evaluation.suggestionScore;
    case IngredientsListRole: {
        return item.recipe.ingredientsVariantList(item.recipe.servings(), [this](const QString &name) {
            return m_pantry ? m_pantry->isIngredientAvailable(name) : false;
        });
    }
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> RecipeModel::roleNames() const {
    return {
        {IdRole, "id"},
        {DishNameRole, "dishName"},
        {CategoryRole, "category"},
        {ServingsRole, "servings"},
        {InstructionsRole, "instructions"},
        {IsStarredRole, "isStarred"},
        {CanMakeRole, "canMake"},
        {MissingCountRole, "missingCount"},
        {MissingIngredientsRole, "missingIngredients"},
        {MissingSummaryRole, "missingSummary"},
        {IsTimeMatchRole, "isTimeMatch"},
        {SuggestionScoreRole, "suggestionScore"},
        {IngredientsListRole, "ingredientsList"}
    };
}

void RecipeModel::setOnlyReadyToCook(bool enabled) {
    if (m_onlyReadyToCook != enabled) {
        m_onlyReadyToCook = enabled;
        applyFilterAndSort();
        emit onlyReadyToCookChanged();
        emit countChanged();
    }
}

void RecipeModel::setOnlyTimeMatch(bool enabled) {
    if (m_onlyTimeMatch != enabled) {
        m_onlyTimeMatch = enabled;
        applyFilterAndSort();
        emit onlyTimeMatchChanged();
        emit countChanged();
    }
}

void RecipeModel::setOnlyStarred(bool enabled) {
    if (m_onlyStarred != enabled) {
        m_onlyStarred = enabled;
        applyFilterAndSort();
        emit onlyStarredChanged();
        emit countChanged();
    }
}

void RecipeModel::setCategoryFilter(const QString &cat) {
    if (m_categoryFilter != cat) {
        m_categoryFilter = cat;
        applyFilterAndSort();
        emit categoryFilterChanged();
        emit countChanged();
    }
}

void RecipeModel::setSearchQuery(const QString &query) {
    if (m_searchQuery != query) {
        m_searchQuery = query;
        applyFilterAndSort();
        emit searchQueryChanged();
        emit countChanged();
    }
}

void RecipeModel::setSortMode(int mode) {
    if (m_sortMode != mode) {
        m_sortMode = mode;
        applyFilterAndSort();
        emit sortModeChanged();
    }
}

QStringList RecipeModel::availableCategories() const {
    QSet<QString> cats;
    if (m_repo) {
        for (const auto &r : m_repo->recipes()) {
            if (!r.category().isEmpty()) {
                cats.insert(r.category());
            }
        }
    }
    QStringList list = cats.values();
    std::sort(list.begin(), list.end(), [](const QString &a, const QString &b) {
        return a.localeAwareCompare(b) < 0;
    });
    list.prepend("Alle");
    return list;
}

void RecipeModel::toggleStarred(const QString &recipeId) {
    if (m_settings) {
        m_settings->toggleRecipeStarred(recipeId);
    }
}

QVariantMap RecipeModel::getRecipe(int index) const {
    if (index < 0 || index >= m_visibleItems.size()) {
        return QVariantMap();
    }
    return getRecipeById(m_visibleItems.at(index).recipe.id());
}

QVariantMap RecipeModel::getRecipeById(const QString &id) const {
    if (!m_repo) return QVariantMap();
    const Recipe r = m_repo->findById(id);
    if (r.dishName().isEmpty()) return QVariantMap();

    RecipeEvaluation eval;
    if (m_engine) {
        eval = m_engine->evaluateRecipe(r);
    }

    QVariantMap map;
    map["id"] = r.id();
    map["dishName"] = r.dishName();
    map["category"] = r.category();
    map["servings"] = r.servings();
    map["instructions"] = r.instructions();
    map["isStarred"] = r.isStarred();
    map["canMake"] = eval.canMake;
    map["missingCount"] = eval.missingIngredientsCount;
    map["missingIngredients"] = eval.missingIngredients;
    map["isTimeMatch"] = eval.isTimeMatch;
    map["suggestionScore"] = eval.suggestionScore;
    map["ingredientsList"] = getScaledIngredients(r.id(), r.servings());

    return map;
}

QVariantList RecipeModel::getScaledIngredients(const QString &id, int targetServings) const {
    if (!m_repo) return QVariantList();
    const Recipe r = m_repo->findById(id);
    if (r.dishName().isEmpty()) return QVariantList();

    return r.ingredientsVariantList(targetServings, [this](const QString &name) {
        return m_pantry ? m_pantry->isIngredientAvailable(name) : false;
    });
}
