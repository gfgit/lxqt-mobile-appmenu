#include "menuappmodel.h"
#include "menuappmap.h"

MenuAppModel::MenuAppModel(QObject *parent)
    : QAbstractListModel(parent)
    , mCurrentCategory(0)
    , mInSearch(false)
{
}

QVariant MenuAppModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant();
}

int MenuAppModel::rowCount(const QModelIndex &p) const
{
    if(!mAppMap || p.isValid() || mCurrentCategory < 0 || mCurrentCategory >= mAppMap->getCategoriesCount())
        return 0;

    if(mInSearch)
        return mSearchMatches.size();

    if(mCurrentCategory == 1)
        return mAppMap->getTotalAppCount(); //Special "All Applications" category

    return mAppMap->getCategoryAt(mCurrentCategory).apps.size();
}

QVariant MenuAppModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid())
        return QVariant();

    const MenuAppMap::AppItem* item = getAppAt(idx.row());
    if(!item)
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
        return item->title;
    case Qt::EditRole:
        return item->desktopFile;
    case Qt::DecorationRole:
        return item->icon;
    case Qt::ToolTipRole:
    {
        QString arr = item->comment + QLatin1String("\n\n") + item->keywords.join(QLatin1Char('\n'));
        return arr;
    }
    default:
        break;
    }

    return QVariant();
}

void MenuAppModel::reloadAppMap(bool end)
{
    if(!end)
        beginResetModel();
    else
        endResetModel();
}

void MenuAppModel::setCurrentCategory(int category)
{
    beginResetModel();
    mCurrentCategory = category;
    endResetModel();
}

void MenuAppModel::showSearchResults(const QVector<const MenuAppItem *> &matches)
{
    beginResetModel();
    mSearchMatches = matches;
    mInSearch = true;
    endResetModel();
}

void MenuAppModel::endSearch()
{
    beginResetModel();
    mSearchMatches.clear();
    mSearchMatches.squeeze();
    mInSearch = false;
    endResetModel();
}

MenuAppMap *MenuAppModel::appMap() const
{
    return mAppMap;
}

void MenuAppModel::setAppMap(MenuAppMap *newAppMap)
{
    mAppMap = newAppMap;
}

const MenuAppItem *MenuAppModel::getAppAt(int idx) const
{
    if(!mAppMap || idx < 0 || mCurrentCategory < 0 || mCurrentCategory >= mAppMap->getCategoriesCount())
        return nullptr;

    if(mInSearch)
        return mSearchMatches.value(idx, nullptr);

    if(mCurrentCategory == 1) //Special "All Applications" category
        return mAppMap->getAppAt(idx);

    return mAppMap->getCategoryAt(mCurrentCategory).apps.value(idx, nullptr);
}
