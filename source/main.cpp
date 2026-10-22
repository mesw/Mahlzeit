#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QClipboard>
#include <QIcon>

#include "services/SettingsManager.h"
#include "services/RecipeRepository.h"
#include "services/SuggestionEngine.h"
#include "services/SyncManager.h"
#include "models/PantryModel.h"
#include "models/ShoppingListModel.h"
#include "models/RecipeModel.h"

class ClipboardHelper : public QObject {
    Q_OBJECT
public:
    explicit ClipboardHelper(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE void copyText(const QString &text) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        if (clipboard) {
            clipboard->setText(text);
        }
    }
};

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    app.setOrganizationName("Malzeit");
    app.setOrganizationDomain("malzeit.app");
    app.setApplicationName("Malzeit");
    app.setApplicationDisplayName("Malzeit");

    SettingsManager settingsManager;
    RecipeRepository recipeRepo(&settingsManager);
    PantryModel pantryModel(&recipeRepo, &settingsManager);
    SuggestionEngine suggestionEngine(&pantryModel, &settingsManager);
    ShoppingListModel shoppingModel(&pantryModel, &recipeRepo, &settingsManager);
    RecipeModel recipeModel(&recipeRepo, &pantryModel, &settingsManager, &suggestionEngine);
    SyncManager syncManager(&settingsManager, &pantryModel);
    ClipboardHelper clipboardHelper;

    QQmlApplicationEngine engine;

    QQmlContext *context = engine.rootContext();
    context->setContextProperty("settingsManager", &settingsManager);
    context->setContextProperty("recipeRepo", &recipeRepo);
    context->setContextProperty("pantryModel", &pantryModel);
    context->setContextProperty("suggestionEngine", &suggestionEngine);
    context->setContextProperty("shoppingModel", &shoppingModel);
    context->setContextProperty("recipeModel", &recipeModel);
    context->setContextProperty("syncManager", &syncManager);
    context->setContextProperty("clipboardHelper", &clipboardHelper);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);

    engine.loadFromModule("Malzeit", "Main");

    return app.exec();
}

#include "main.moc"
