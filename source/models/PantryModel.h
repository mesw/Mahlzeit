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
    QString categoryName;
    int categoryOrder{999};
    QStringList aliases;
};

class PantryModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int totalCount READ totalCount NOTIFY countsChanged)
    Q_PROPERTY(int availableCount READ availableCount NOTIFY countsChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(int filterMode READ filterMode WRITE setFilterMode NOTIFY filterModeChanged) // 0: All, 1: Available, 2: Missing
    Q_PROPERTY(QString categoryFilter READ categoryFilter WRITE setCategoryFilter NOTIFY categoryFilterChanged)
    Q_PROPERTY(int sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged) // 0: A-Z, 1: Store aisle order
    Q_PROPERTY(QStringList availableCategories READ availableCategories NOTIFY categoriesChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        AvailableRole,
        RecipeCountRole,
        CategoryRole,
        CategoryOrderRole,
        AliasesRole
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

    QString categoryFilter() const { return m_categoryFilter; }
    void setCategoryFilter(const QString &category);

    int sortOrder() const { return m_sortOrder; }
    void setSortOrder(int order);

    QStringList availableCategories() const;
    QString canonicalName(const QString &ingredientName) const;

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
    void categoryFilterChanged();
    void sortOrderChanged();
    void categoriesChanged();
    void pantryChanged();
    void itemAvailabilityEdited(const QString &canonicalName, bool available);
    void bulkAvailabilityEdited(const QMap<QString, bool> &changes);

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
    QString m_categoryFilter{QStringLiteral("Alle")};
    int m_sortOrder{0};
};
