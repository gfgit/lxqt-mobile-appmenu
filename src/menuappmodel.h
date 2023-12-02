#ifndef LXQTFANCYMENUAPPMODEL_H
#define LXQTFANCYMENUAPPMODEL_H

#include <QAbstractListModel>

class MenuAppMap;
class MenuAppItem;

class MenuAppModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit MenuAppModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &p = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    void reloadAppMap(bool end);
    void setCurrentCategory(int category);
    void showSearchResults(const QVector<const MenuAppItem *> &matches);
    void endSearch();

    MenuAppMap *appMap() const;
    void setAppMap(MenuAppMap *newAppMap);

    const MenuAppItem *getAppAt(int idx) const;

private:
    MenuAppMap *mAppMap;
    int mCurrentCategory;

    QVector<const MenuAppItem *> mSearchMatches;
    bool mInSearch;
};

#endif // LXQTFANCYMENUAPPMODEL_H
