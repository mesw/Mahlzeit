#include <QTest>
#include <QSignalSpy>
#include <QFile>

#include "models/Recipe.h"
#include "services/SettingsManager.h"
#include "services/RecipeRepository.h"
#include "models/PantryModel.h"
#include "models/ShoppingListModel.h"
#include "services/SuggestionEngine.h"
#include "models/RecipeModel.h"

class TestMahlzeit : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testRecipeJsonParsing();
    void testServingsScaling();
    void testPantryManagement();
    void testAliasesAndCategories();
    void testShoppingListLogic();
    void testSuggestionEngineTimeAndAvailability();
    void cleanupTestCase();

private:
    SettingsManager *m_settings{nullptr};
    RecipeRepository *m_repo{nullptr};
    PantryModel *m_pantry{nullptr};
    SuggestionEngine *m_engine{nullptr};
    ShoppingListModel *m_shopping{nullptr};
    RecipeModel *m_recipeModel{nullptr};
};

void TestMahlzeit::initTestCase() {
    QCoreApplication::setOrganizationName("MahlzeitTest");
    QCoreApplication::setApplicationName("MahlzeitTest");

    m_settings = new SettingsManager(this);
    m_settings->clearAllUserData();

    m_repo = new RecipeRepository(m_settings, this);
    m_pantry = new PantryModel(m_repo, m_settings, this);
    m_engine = new SuggestionEngine(m_pantry, m_settings, this);
    m_shopping = new ShoppingListModel(m_pantry, m_repo, m_settings, this);
    m_recipeModel = new RecipeModel(m_repo, m_pantry, m_settings, m_engine, this);
}

void TestMahlzeit::testRecipeJsonParsing() {
    QVERIFY(m_repo->recipeCount() >= 32);

    Recipe milchreis = m_repo->findById("milchreis-mit-pfirsich-und-honig");
    QCOMPARE(milchreis.dishName(), QString("Milchreis mit Pfirsich und Honig"));
    QCOMPARE(milchreis.category(), QString("Frühstück & Breie"));
    QCOMPARE(milchreis.servings(), 1);
    QVERIFY(milchreis.ingredients().size() >= 5);
}

void TestMahlzeit::testServingsScaling() {
    Recipe hirsebrei = m_repo->findById("hirsebrei"); // 2 servings base, 100g Hirse
    QCOMPARE(hirsebrei.servings(), 2);

    // Scale to 4 servings (double)
    auto scaled4 = hirsebrei.scaledIngredients(4);
    for (const auto &ing : scaled4) {
        if (ing.name == "Hirse") {
            QCOMPARE(ing.amount, 200.0);
        }
    }

    // Scale to 1 serving (half)
    auto scaled1 = hirsebrei.scaledIngredients(1);
    for (const auto &ing : scaled1) {
        if (ing.name == "Hirse") {
            QCOMPARE(ing.amount, 50.0);
        }
    }
}

void TestMahlzeit::testPantryManagement() {
    m_pantry->clearAll();
    QCOMPARE(m_pantry->availableCount(), 0);

    m_pantry->setItemAvailable("Haferflocken", true);
    QVERIFY(m_pantry->isIngredientAvailable("Haferflocken"));
    QCOMPARE(m_pantry->availableCount(), 1);

    m_pantry->setItemAvailable("Haferflocken", false);
    QVERIFY(!m_pantry->isIngredientAvailable("Haferflocken"));
    QCOMPARE(m_pantry->availableCount(), 0);

    m_pantry->selectAll();
    QCOMPARE(m_pantry->availableCount(), m_pantry->totalCount());
}

void TestMahlzeit::testAliasesAndCategories() {
    m_pantry->clearAll();

    // Test canonical alias resolution
    QCOMPARE(m_repo->canonicalIngredientName("Äpfel"), QString("Apfel"));
    QCOMPARE(m_repo->canonicalIngredientName("Dotter"), QString("Ei"));
    QCOMPARE(m_repo->canonicalIngredientName("Eiklar"), QString("Ei"));
    QCOMPARE(m_repo->canonicalIngredientName("Erdäpfel"), QString("Kartoffeln"));

    // Setting an alias available marks the canonical ingredient and all other variants available
    m_pantry->setItemAvailable("Dotter", true);
    QVERIFY(m_pantry->isIngredientAvailable("Ei"));
    QVERIFY(m_pantry->isIngredientAvailable("Eiklar"));
    QVERIFY(m_pantry->isIngredientAvailable("Harte Eier"));
    QCOMPARE(m_pantry->availableCount(), 1);

    // Test store aisle ordering and overrides
    m_settings->setActiveStoreId("stamm-supermarkt");
    const int produceOrderMain = m_repo->ingredientCategoryOrder("Apfel");
    const int breadOrderMain = m_repo->ingredientCategoryOrder("Haferflocken");
    QVERIFY(produceOrderMain < breadOrderMain);

    m_settings->setActiveStoreId("discounter");
    const int produceOrderDisc = m_repo->ingredientCategoryOrder("Apfel");
    const int breadOrderDisc = m_repo->ingredientCategoryOrder("Haferflocken");
    QVERIFY(breadOrderDisc < produceOrderDisc);
    QCOMPARE(m_repo->ingredientCategoryId("Haltbarmilch", "discounter"), QString("chilled"));

    m_settings->setActiveStoreId("stamm-supermarkt");
}

void TestMahlzeit::testShoppingListLogic() {
    m_pantry->clearAll();
    m_shopping->rebuildList();

    // Since pantry is empty, all ingredients should be on the shopping list
    QVERIFY(m_shopping->itemCount() > 0);

    // Star a recipe (Milchreis)
    const QString recipeId = "milchreis-mit-pfirsich-und-honig";
    m_settings->setRecipeStarred(recipeId, true);
    QVERIFY(m_settings->isRecipeStarred(recipeId));

    // Filter by starred only
    m_shopping->setOnlyStarred(true);
    QVERIFY(m_shopping->itemCount() > 0);

    // Buying Magermilch should add it to pantry
    m_shopping->markPurchasedByName("Magermilch");
    QVERIFY(m_pantry->isIngredientAvailable("Magermilch"));
}

void TestMahlzeit::testSuggestionEngineTimeAndAvailability() {
    m_pantry->clearAll();

    Recipe milchreis = m_repo->findById("milchreis-mit-pfirsich-und-honig");

    // All ingredients missing initially
    auto eval1 = m_engine->evaluateRecipe(milchreis);
    QVERIFY(!eval1.canMake);
    QVERIFY(eval1.missingIngredientsCount > 0);

    // Add all ingredients of Milchreis to pantry
    for (const auto &ing : milchreis.ingredients()) {
        m_pantry->setItemAvailable(ing.name, true);
    }

    auto eval2 = m_engine->evaluateRecipe(milchreis);
    QVERIFY(eval2.canMake);
    QCOMPARE(eval2.missingIngredientsCount, 0);

    // Test time matching at 07:00 (Breakfast time)
    m_engine->setSimulatedTime(QTime(7, 0));
    m_engine->setUseSimulatedTime(true);

    auto evalMorning = m_engine->evaluateRecipe(milchreis);
    QVERIFY(evalMorning.isTimeMatch);

    // Test time matching at 23:30 (Not breakfast time)
    m_engine->setSimulatedTime(QTime(23, 30));
    auto evalNight = m_engine->evaluateRecipe(milchreis);
    QVERIFY(!evalNight.isTimeMatch);
}

void TestMahlzeit::cleanupTestCase() {
    m_settings->clearAllUserData();
}

QTEST_MAIN(TestMahlzeit)
#include "test_mahlzeit.moc"
