#include "menuappmap.h"

#include <XdgMenu>

MenuAppMap::MenuAppMap()
{
    mCachedIndex = -1;
    mCachedIterator = mAppSortedByName.constEnd();
}

MenuAppMap::~MenuAppMap()
{
    clear();
}

void MenuAppMap::clear()
{
    mCategories.clear();
    mAppSortedByName.clear();
    qDeleteAll(mAppSortedByDesktopFile);
    mAppSortedByDesktopFile.clear();

    mCachedIndex = -1;
    mCachedIterator = mAppSortedByName.constEnd();
}

bool MenuAppMap::rebuildModel(const XdgMenu &menu)
{
    clear();

    //Add Favorites category
    Category favorites;
    favorites.menuTitle = QLatin1String("Favorites"); //TODO: translate
    favorites.icon = QIcon::fromTheme(QLatin1String("bookmarks"));
    mCategories.append(favorites);

    //Add All Apps category
    Category allAppsCategory;
    allAppsCategory.menuTitle = QLatin1String("All Applications");
    allAppsCategory.icon = QIcon::fromTheme(QLatin1String("folder"));
    mCategories.append(allAppsCategory);

    //TODO: add separator

    QDomElement rootMenu = menu.xml().documentElement();
    parseMenu(rootMenu, QString());
    return true;
}

MenuAppMap::AppItem *MenuAppMap::getAppAt(int index)
{
    if(index < 0 || index >= getTotalAppCount())
        return nullptr;

    if(mCachedIndex != -1)
    {
        if(index == mCachedIndex + 1)
        {
            //Fast case, go to next row
            mCachedIndex++;
            mCachedIterator++;
        }

        if(index == mCachedIndex)
            return *mCachedIterator;

        int dist1 = qAbs(mCachedIndex - index);
        if(dist1 < index)
        {
            std::advance(mCachedIterator, index - mCachedIndex);
            mCachedIndex = index;
            return *mCachedIterator;
        }
    }

    // Recalculate cached iterator
    mCachedIterator = mAppSortedByName.constBegin();
    std::advance(mCachedIterator, index);
    mCachedIndex = index;
    return *mCachedIterator;
}

QVector<const MenuAppMap::AppItem *> MenuAppMap::getMatchingApps(const QString &query) const
{
    QVector<const AppItem *> byName;
    QVector<const AppItem *> byKeyword;

    //TODO: implement some kind of score to get better matches on top

    for(const AppItem *app : qAsConst(mAppSortedByName))
    {
        if(app->title.contains(query))
        {
            byName.append(app);
            continue;
        }

        if(app->comment.contains(query))
        {
            byKeyword.append(app);
            continue;
        }

        for(const QString& key : app->keywords)
        {
            if(key.startsWith(query))
            {
                byKeyword.append(app);
                break;
            }
        }
    }

    // Give priority to title matches
    byName += byKeyword;

    return byName;
}

void MenuAppMap::parseMenu(const QDomElement &menu, const QString& topLevelCategory)
{
    QDomElement e = menu.firstChildElement();
    while(!e.isNull())
    {
        if(e.tagName() == QLatin1String("Menu"))
        {
            if(topLevelCategory.isEmpty())
            {
                //This is a top level menu
                Category item;
                item.menuName = e.attribute(QLatin1String("name"));
                item.menuTitle = e.attribute(QLatin1Literal("title"), item.menuName);
                QString iconName = e.attribute(QLatin1String("icon"));
                item.icon = QIcon::fromTheme(iconName);
                mCategories.append(item);

                //Merge sub menu to parent
                parseMenu(e, item.menuName);
            }
            else
            {
                //Merge sub menu to parent
                parseMenu(e, topLevelCategory);
            }
        }
        else if(!topLevelCategory.isEmpty() && e.tagName() == QLatin1String("AppLink"))
        {
            parseAppLink(e, topLevelCategory);
        }

        e = e.nextSiblingElement();
    }
}

void MenuAppMap::parseAppLink(const QDomElement &app, const QString& topLevelCategory)
{
    QString desktopFile = app.attribute(QLatin1String("desktopFile"));

    // Check if already added
    AppItem *item = mAppSortedByDesktopFile.value(desktopFile, nullptr);
    if(!item)
    {
        // Add new app

        XdgDesktopFile f;
        if(!f.load(desktopFile))
            return; // Invalid App

        item = new AppItem;
        item->desktopFile = desktopFile;
        item->title = app.attribute(QLatin1Literal("title"));
        item->comment = app.attribute(QLatin1String("comment"));
        if(item->comment.isEmpty())
            item->comment = f.value(QLatin1String("GenericName")).toString();
        QString iconName = app.attribute(QLatin1String("icon"));
        item->icon = QIcon::fromTheme(iconName);
        item->desktopFileCache = f;

        item->keywords << f.value(QLatin1String("Keywords")).toString().toLower().split(QLatin1Char(';'));
        item->keywords.append(item->title.toLower().split(QLatin1Char(' ')));
        item->keywords.append(item->comment.toLower().split(QLatin1Char(' ')));

        mAppSortedByDesktopFile.insert(item->desktopFile, item);
        mAppSortedByName.insert(item->title, item);
    }

    // Now add app to category
    for(Category &category : mCategories)
    {
        if(category.menuName == topLevelCategory)
        {
            category.apps.append(item);
            break;
        }
    }
}
