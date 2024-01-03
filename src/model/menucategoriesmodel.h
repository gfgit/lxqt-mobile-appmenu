#ifndef LXQTFANCYMENUCATEGORIESMODEL_H
#define LXQTFANCYMENUCATEGORIESMODEL_H

#include <QAbstractListModel>

class MenuAppMap;

class MenuCategoriesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit MenuCategoriesModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &p = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    void reloadAppMap(bool end);

    MenuAppMap *appMap() const;
    void setAppMap(MenuAppMap *newAppMap);

private:
    MenuAppMap *mAppMap;
};

#endif // LXQTFANCYMENUCATEGORIESMODEL_H
