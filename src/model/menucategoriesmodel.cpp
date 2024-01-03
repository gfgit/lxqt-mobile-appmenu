#include "menucategoriesmodel.h"
#include "menuappmap.h"

MenuCategoriesModel::MenuCategoriesModel(QObject *parent)
    : QAbstractListModel(parent)
    , mAppMap(nullptr)
{
}

QVariant MenuCategoriesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant();
}

int MenuCategoriesModel::rowCount(const QModelIndex &p) const
{
    if(!mAppMap || p.isValid())
        return 0;

    return mAppMap->getCategoriesCount();
}

QVariant MenuCategoriesModel::data(const QModelIndex &idx, int role) const
{
    if (!mAppMap || !idx.isValid() || idx.row() >= mAppMap->getCategoriesCount())
        return QVariant();

    const MenuAppMap::Category& item = mAppMap->getCategoryAt(idx.row());

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return item.menuTitle;
    case Qt::EditRole:
        return item.menuName;
    case Qt::DecorationRole:
        return item.icon;
    default:
        break;
    }

    return QVariant();
}

void MenuCategoriesModel::reloadAppMap(bool end)
{
    if(!end)
        beginResetModel();
    else
        endResetModel();
}

MenuAppMap *MenuCategoriesModel::appMap() const
{
    return mAppMap;
}

void MenuCategoriesModel::setAppMap(MenuAppMap *newAppMap)
{
    mAppMap = newAppMap;
}
