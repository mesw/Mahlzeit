#include "RecipeRepository.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QUrl>
#include <QNetworkRequest>

static const QString REMOTE_RECIPES_URL = QStringLiteral("https://mesw.github.io/Malzeit/res/recipies.json");

RecipeRepository::RecipeRepository(SettingsManager *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    loadRecipes();

    if (m_settings) {
        connect(m_settings, &SettingsManager::starredChanged, this, [this](const QString &id, bool starred) {
            for (auto &r : m_recipes) {
                if (r.id() == id) {
                    r.setStarred(starred);
                    break;
                }
            }
            emit recipesChanged();
        });

        connect(m_settings, &SettingsManager::userDataReset, this, [this]() {
            resetToBundledDefaults();
        });
    }
}

void RecipeRepository::loadRecipes() {
    // 1. Try to load from user settings cache first
    if (!loadFromLocalCache()) {
        // 2. First run: load from bundled resource and save to user settings
        loadFromResource();
        saveToLocalCache();
    }

    // Apply starred states
    if (m_settings) {
        for (auto &r : m_recipes) {
            r.setStarred(m_settings->isRecipeStarred(r.id()));
        }
    }

    emit recipesChanged();
}

Recipe RecipeRepository::findById(const QString &id) const {
    for (const auto &r : m_recipes) {
        if (r.id() == id) {
            return r;
        }
    }
    return Recipe();
}

bool RecipeRepository::parseJsonData(const QByteArray &data, QList<Recipe> &outRecipes) {
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        qWarning() << "Failed to parse recipes JSON:" << err.errorString();
        return false;
    }

    outRecipes.clear();
    const QJsonArray arr = doc.array();
    for (const auto &val : arr) {
        if (val.isObject()) {
            Recipe r = Recipe::fromJsonObject(val.toObject());
            if (!r.dishName().isEmpty()) {
                outRecipes.append(r);
            }
        }
    }
    return !outRecipes.isEmpty();
}

void RecipeRepository::loadFromResource() {
    const QStringList candidatePaths = {
        ":/resources/recipes.json",
        ":/resources/recipies.json",
        ":/recipes.json",
        ":/recipies.json",
        ":/qt/qml/Malzeit/resources/recipes.json",
        ":/qt/qml/Malzeit/recipes.json",
        "resources/recipes.json",
        "resources/recipies.json",
        "../resources/recipes.json"
    };

    QByteArray data;
    for (const auto &path : candidatePaths) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            data = file.readAll();
            file.close();
            if (!data.isEmpty()) {
                qDebug() << "Loaded bundled recipes from:" << path;
                break;
            }
        }
    }

    if (data.isEmpty()) {
        qWarning() << "Could not open bundled recipes.json from any known path!";
        return;
    }

    QList<Recipe> loaded;
    if (parseJsonData(data, loaded)) {
        m_recipes = loaded;
    }
}

bool RecipeRepository::loadFromLocalCache() {
    QSettings settings("Malzeit", "Malzeit");
    const QString json = settings.value("cachedRecipesJson").toString();
    if (json.isEmpty()) {
        return false;
    }

    QList<Recipe> loaded;
    if (parseJsonData(json.toUtf8(), loaded)) {
        m_recipes = loaded;
        return true;
    }
    return false;
}

void RecipeRepository::saveToLocalCache() {
    QJsonArray arr;
    for (const auto &r : m_recipes) {
        arr.append(r.toJsonObject());
    }
    const QJsonDocument doc(arr);
    QSettings settings("Malzeit", "Malzeit");
    settings.setValue("cachedRecipesJson", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    settings.sync();
}

void RecipeRepository::resetToBundledDefaults() {
    loadFromResource();
    saveToLocalCache();
    if (m_settings) {
        for (auto &r : m_recipes) {
            r.setStarred(m_settings->isRecipeStarred(r.id()));
        }
    }
    m_lastRefreshStatus = tr("Rezepte auf Standard zurückgesetzt.");
    emit lastRefreshStatusChanged();
    emit recipesChanged();
    emit refreshFinished(true, m_lastRefreshStatus);
}

void RecipeRepository::updateRecipeInstructions(const QString &recipeId, const QString &markdown) {
    for (auto &r : m_recipes) {
        if (r.id() == recipeId) {
            r.setInstructions(markdown);
            break;
        }
    }
    saveToLocalCache();
    emit recipesChanged();
}

void RecipeRepository::refreshFromWeb() {
    if (m_isRefreshing) return;

    m_isRefreshing = true;
    emit isRefreshingChanged();
    m_lastRefreshStatus = tr("Lade Rezepte von Server...");
    emit lastRefreshStatusChanged();

    const QUrl requestUrl(REMOTE_RECIPES_URL);
    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Malzeit-App/1.0");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_isRefreshing = false;
        emit isRefreshingChanged();

        if (reply->error() != QNetworkReply::NoError) {
            m_lastRefreshStatus = tr("Fehler beim Laden: %1").arg(reply->errorString());
            emit lastRefreshStatusChanged();
            emit refreshFinished(false, m_lastRefreshStatus);
            return;
        }

        const QByteArray responseData = reply->readAll();
        QList<Recipe> newRecipes;
        if (!parseJsonData(responseData, newRecipes)) {
            m_lastRefreshStatus = tr("Ungültiges JSON-Format von Server empfangen.");
            emit lastRefreshStatusChanged();
            emit refreshFinished(false, m_lastRefreshStatus);
            return;
        }

        // Preserve local instructions and stars
        for (auto &nr : newRecipes) {
            const Recipe old = findById(nr.id());
            if (!old.dishName().isEmpty()) {
                if (nr.instructions().isEmpty() && !old.instructions().isEmpty()) {
                    nr.setInstructions(old.instructions());
                }
            }
            if (m_settings) {
                nr.setStarred(m_settings->isRecipeStarred(nr.id()));
            }
        }

        m_recipes = newRecipes;
        saveToLocalCache();

        m_lastRefreshStatus = tr("%1 Rezepte erfolgreich aktualisiert!").arg(m_recipes.size());
        emit lastRefreshStatusChanged();
        emit recipesChanged();
        emit refreshFinished(true, m_lastRefreshStatus);
    });
}
