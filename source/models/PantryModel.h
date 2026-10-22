#pragma once

#include <QAbstractListModel>
#include <QSet>
#include <QMap>
#include <QStringList>
#include "Recipe.h"
#include "services/SettingsManager.h"
#include "services/RecipeRepository.h"

struct PantryItem {
    QString name;
    bool available{false};
    int recipeCount{0};
};

class PantryModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int totalCount READ totalCount NOTIFY countsChanged)
    Q_PROPERTY(int availableCount READ availableCount NOTIFY countsChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(int filterMode READ filterMode WRITE setFilterMode NOTIFY filterModeChanged) // 0: All, 1: Available, 2: Missing

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        AvailableRole,
        RecipeCountRole
    };
    Q_ENUM(Roles)

    explicit PantryModel(RecipeRepository *repo, SettingsManager *settings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int totalCount() const { return m_allItems.size(); }
    int availableCount() const;

    QString filterText() const { return m_filterText; }
    void setFilterText(const QString &text);

    int filterMode() const { return m_filterMode; }
    void setFilterMode(int mode);

    Q_INVOKABLE void toggleAvailable(int visibleIndex);
    Q_INVOKABLE void setItemAvailable(const QString &ingredientName, bool available);
    Q_INVOKABLE bool isIngredientAvailable(const QString &ingredientName) const;
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearAll();

    QMap<QString, bool> getPantryMap() const;
    void applyPantryMap(const QMap<QString, bool> &map);

signals:
    void countsChanged();
    void filterTextChanged();
    void filterModeChanged();
    void pantryChanged();

private slots:
    void rebuildIngredientList();

private:
    void updateFilteredList();
    void saveToSettings();

    RecipeRepository *m_repo{nullptr};
    SettingsManager *m_settings{nullptr};

    QList<PantryItem> m_allItems;
    QList<PantryItem> m_filteredItems;
    QMap<QString, bool> m_availabilityMap;

    QString m_filterText;
    int m_filterMode{0};
};
