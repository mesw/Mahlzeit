#include "Recipe.h"
#include <QRegularExpression>

static QString inferUnit(const QString &name) {
    const QString lower = name.toLower();
    if (lower.contains("milch") || lower.contains("wasser") || lower.contains("saft") ||
        lower.contains("öl") || lower.contains("oel") || lower.contains("bouillon") ||
        lower.contains("essig") || lower.contains("sauce") || lower.contains("soße") ||
        lower.contains("sugo")) {
        return "ml";
    }
    return "g";
}

static QString slugify(const QString &text) {
    QString slug = text.toLower();
    slug.replace("ä", "ae");
    slug.replace("ö", "oe");
    slug.replace("ü", "ue");
    slug.replace("ß", "ss");
    slug.replace(QRegularExpression("[^a-z0-9]+"), "-");
    while (slug.startsWith('-')) slug.remove(0, 1);
    while (slug.endsWith('-')) slug.chop(1);
    return slug.isEmpty() ? "recipe" : slug;
}

QList<RecipeIngredient> Recipe::scaledIngredients(int targetServings) const {
    if (targetServings <= 0 || targetServings == m_servings) {
        return m_ingredients;
    }
    QList<RecipeIngredient> result;
    result.reserve(m_ingredients.size());
    const double ratio = static_cast<double>(targetServings) / static_cast<double>(m_servings);
    for (const auto &ing : m_ingredients) {
        result.append(RecipeIngredient{
            ing.name,
            ing.amount * ratio,
            ing.unit
        });
    }
    return result;
}

QVariantList Recipe::ingredientsVariantList(int targetServings, const std::function<bool(const QString &)> &inPantryChecker) const {
    QVariantList list;
    list.reserve(m_ingredients.size());
    const int effectiveServings = targetServings > 0 ? targetServings : m_servings;
    for (const auto &ing : m_ingredients) {
        const bool inPantry = inPantryChecker ? inPantryChecker(ing.name) : false;
        list.append(ing.toVariantMap(effectiveServings, m_servings, inPantry));
    }
    return list;
}

Recipe Recipe::fromJsonObject(const QJsonObject &obj) {
    Recipe r;
    r.setDishName(obj.value("dish_name").toString());
    r.setCategory(obj.value("category").toString());
    r.setServings(obj.value("servings").toInt(1));
    r.setInstructions(obj.value("instructions").toString());

    QString id = obj.value("id").toString();
    if (id.isEmpty()) {
        id = slugify(r.dishName());
    }
    r.setId(id);

    QList<RecipeIngredient> ings;

    if (obj.contains("ingredient_details") && obj.value("ingredient_details").isArray()) {
        const QJsonArray arr = obj.value("ingredient_details").toArray();
        for (const auto &v : arr) {
            const QJsonObject o = v.toObject();
            RecipeIngredient ing;
            ing.name = o.value("name").toString();
            ing.amount = o.value("amount").toDouble();
            ing.unit = o.value("unit").toString();
            if (ing.unit.isEmpty()) {
                ing.unit = inferUnit(ing.name);
            }
            if (!ing.name.isEmpty()) {
                ings.append(ing);
            }
        }
    }

    r.setIngredients(ings);
    return r;
}

QJsonObject Recipe::toJsonObject() const {
    QJsonObject obj;
    obj["id"] = m_id;
    obj["dish_name"] = m_dishName;
    obj["category"] = m_category;
    obj["servings"] = m_servings;
    obj["instructions"] = m_instructions;

    QJsonArray detailsArr;
    for (const auto &ing : m_ingredients) {
        QJsonObject d;
        d["name"] = ing.name;
        d["amount"] = ing.amount;
        d["unit"] = ing.unit;
        detailsArr.append(d);
    }
    obj["ingredient_details"] = detailsArr;

    return obj;
}
