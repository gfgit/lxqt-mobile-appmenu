#ifndef LXQTFANCYMENUAPPMAP_H
#define LXQTFANCYMENUAPPMAP_H

#include <QString>
#include <QIcon>
#include <QMap>
#include <QVector>

#include <XdgDesktopFile>

class XdgMenu;
class QDomElement;

struct MenuAppItem
{
    QString desktopFile;
    QString title;
    QString comment;
    QIcon icon;
    QStringList keywords;
    XdgDesktopFile desktopFileCache;
};

class MenuAppMap
{
public:
    typedef MenuAppItem AppItem;

    struct Category
    {
        QString menuName;
        QString menuTitle;
        QIcon icon;
        QVector<AppItem *> apps;
    };

    MenuAppMap();
    ~MenuAppMap();

    void clear();
    bool rebuildModel(const XdgMenu &menu);

    inline int getCategoriesCount() const { return mCategories.size(); }
    inline const Category& getCategoryAt(int index) { return mCategories.at(index); }

    inline int getTotalAppCount() const { return mAppSortedByName.size(); }

    AppItem *getAppAt(int index);

    QVector<const AppItem *> getMatchingApps(const QString& query) const;

private:
    void parseMenu(const QDomElement& menu, const QString &topLevelCategory);
    void parseAppLink(const QDomElement& app, const QString &topLevelCategory);

private:
    typedef QMap<QString, AppItem *> AppMap;
    AppMap mAppSortedByDesktopFile;
    AppMap mAppSortedByName;
    QVector<Category> mCategories;

    // Cache sort by name map access
    AppMap::const_iterator mCachedIterator;
    int mCachedIndex;
};

#endif // LXQTFANCYMENUAPPMAP_H
