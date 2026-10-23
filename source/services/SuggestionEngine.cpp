#include "SuggestionEngine.h"

SuggestionEngine::SuggestionEngine(PantryModel *pantry, SettingsManager *settings, QObject *parent)
    : QObject(parent)
    , m_pantry(pantry)
    , m_settings(settings)
{
    if (m_pantry) {
        connect(m_pantry, &PantryModel::pantryChanged, this, &SuggestionEngine::evaluationChanged);
    }
    if (m_settings) {
        connect(m_settings, &SettingsManager::timeSlotsChanged, this, &SuggestionEngine::evaluationChanged);
        connect(m_settings, &SettingsManager::starredChanged, this, &SuggestionEngine::evaluationChanged);
    }
}

QTime SuggestionEngine::effectiveTime() const {
    if (m_useSimulatedTime && m_simulatedTime.isValid()) {
        return m_simulatedTime;
    }
    return QTime::currentTime();
}

RecipeEvaluation SuggestionEngine::evaluateRecipe(const Recipe &recipe) const {
    RecipeEvaluation eval;
    const auto &ings = recipe.ingredients();
    eval.totalIngredients = ings.size();

    for (const auto &ing : ings) {
        const QString name = ing.name.trimmed();
        const bool available = m_pantry ? m_pantry->isIngredientAvailable(name) : false;
        if (!available) {
            const QString canon = m_pantry ? m_pantry->canonicalName(name) : name;
            if (!canon.isEmpty() && !eval.missingIngredients.contains(canon)) {
                eval.missingIngredients.append(canon);
            }
        }
    }

    eval.missingIngredientsCount = eval.missingIngredients.size();
    eval.canMake = (eval.missingIngredientsCount == 0);

    const QTime now = effectiveTime();
    eval.isTimeMatch = m_settings ? m_settings->isCategoryActiveAtTime(recipe.category(), now) : true;

    // Calculate score
    int score = 0;
    if (eval.canMake) {
        score += 1000;
    } else {
        score -= (eval.missingIngredientsCount * 50);
    }

    if (eval.isTimeMatch) {
        score += 500;
    }

    if (recipe.isStarred()) {
        score += 200;
    }

    eval.suggestionScore = score;
    return eval;
}

void SuggestionEngine::setSimulatedTime(const QTime &time) {
    if (m_simulatedTime != time) {
        m_simulatedTime = time;
        emit simulatedTimeChanged();
        emit evaluationChanged();
    }
}

void SuggestionEngine::setUseSimulatedTime(bool use) {
    if (m_useSimulatedTime != use) {
        m_useSimulatedTime = use;
        emit useSimulatedTimeChanged();
        emit evaluationChanged();
    }
}
