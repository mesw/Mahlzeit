#pragma once

#include <QObject>
#include <QTime>
#include "models/Recipe.h"
#include "models/PantryModel.h"
#include "SettingsManager.h"

struct RecipeEvaluation {
    bool canMake{false};
    int totalIngredients{0};
    int missingIngredientsCount{0};
    QStringList missingIngredients;
    bool isTimeMatch{false};
    int suggestionScore{0};
};

class SuggestionEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(QTime simulatedTime READ simulatedTime WRITE setSimulatedTime NOTIFY simulatedTimeChanged)
    Q_PROPERTY(bool useSimulatedTime READ useSimulatedTime WRITE setUseSimulatedTime NOTIFY useSimulatedTimeChanged)

public:
    explicit SuggestionEngine(PantryModel *pantry, SettingsManager *settings, QObject *parent = nullptr);

    QTime effectiveTime() const;

    RecipeEvaluation evaluateRecipe(const Recipe &recipe) const;

    QTime simulatedTime() const { return m_simulatedTime; }
    void setSimulatedTime(const QTime &time);

    bool useSimulatedTime() const { return m_useSimulatedTime; }
    void setUseSimulatedTime(bool use);

signals:
    void simulatedTimeChanged();
    void useSimulatedTimeChanged();
    void evaluationChanged();

private:
    PantryModel *m_pantry{nullptr};
    SettingsManager *m_settings{nullptr};
    QTime m_simulatedTime{QTime::currentTime()};
    bool m_useSimulatedTime{false};
};
