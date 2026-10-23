#pragma once

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>

struct RecipeIngredient {
    QString name;
    double amount{0.0};
    QString unit{"g"};

    QVariantMap toVariantMap(int targetServings = 0, int baseServings = 1, bool inPantry = false) const {
        double scaledAmount = amount;
        if (targetServings > 0 && baseServings > 0) {
            scaledAmount = (amount * static_cast<double>(targetServings)) / static_cast<double>(baseServings);
        }
        return QVariantMap{
            {"name", name},
            {"amount", scaledAmount},
            {"unit", unit},
            {"inPantry", inPantry}
        };
    }
};

class Recipe {
public:
    Recipe() = default;

    QString id() const { return m_id; }
    void setId(const QString &id) { m_id = id; }

    QString dishName() const { return m_dishName; }
    void setDishName(const QString &name) { m_dishName = name; }

    QString category() const { return m_category; }
    void setCategory(const QString &category) { m_category = category; }

    int servings() const { return m_servings; }
    void setServings(int s) { m_servings = s > 0 ? s : 1; }

    QList<RecipeIngredient> ingredients() const { return m_ingredients; }
    void setIngredients(const QList<RecipeIngredient> &list) { m_ingredients = list; }

    QList<RecipeIngredient> ingredientDetails() const { return m_ingredients; }
    void setIngredientDetails(const QList<RecipeIngredient> &list) { m_ingredients = list; }

    QString instructions() const { return m_instructions; }
    void setInstructions(const QString &instructions) { m_instructions = instructions; }

    bool isStarred() const { return m_isStarred; }
    void setStarred(bool starred) { m_isStarred = starred; }

    QList<RecipeIngredient> scaledIngredients(int targetServings) const;
    QVariantList ingredientsVariantList(int targetServings, const std::function<bool(const QString &)> &inPantryChecker) const;

    static Recipe fromJsonObject(const QJsonObject &obj);
    QJsonObject toJsonObject() const;

private:
    QString m_id;
    QString m_dishName;
    QString m_category;
    int m_servings{1};
    QList<RecipeIngredient> m_ingredients;
    QString m_instructions;
    bool m_isStarred{false};
};
