#pragma once

#include <QObject>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "models/Recipe.h"
#include "SettingsManager.h"

class RecipeRepository : public QObject {
    Q_OBJECT
    Q_PROPERTY(int recipeCount READ recipeCount NOTIFY recipesChanged)
    Q_PROPERTY(bool isRefreshing READ isRefreshing NOTIFY isRefreshingChanged)
    Q_PROPERTY(QString lastRefreshStatus READ lastRefreshStatus NOTIFY lastRefreshStatusChanged)

public:
    explicit RecipeRepository(SettingsManager *settings, QObject *parent = nullptr);

    void loadRecipes();

    QList<Recipe> recipes() const { return m_recipes; }
    int recipeCount() const { return m_recipes.size(); }
    Recipe findById(const QString &id) const;

    Q_INVOKABLE void refreshFromWeb();
    Q_INVOKABLE void resetToBundledDefaults();
    Q_INVOKABLE void updateRecipeInstructions(const QString &recipeId, const QString &markdown);

    bool isRefreshing() const { return m_isRefreshing; }
    QString lastRefreshStatus() const { return m_lastRefreshStatus; }

signals:
    void recipesChanged();
    void isRefreshingChanged();
    void lastRefreshStatusChanged();
    void refreshFinished(bool success, const QString &message);

private:
    void loadFromResource();
    void saveToLocalCache();
    bool loadFromLocalCache();
    bool parseJsonData(const QByteArray &data, QList<Recipe> &outRecipes);

    SettingsManager *m_settings{nullptr};
    QList<Recipe> m_recipes;
    QNetworkAccessManager m_networkManager;
    bool m_isRefreshing{false};
    QString m_lastRefreshStatus;
};
