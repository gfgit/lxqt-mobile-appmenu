#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QString>

const QLatin1String defEnv("X-LXQT;LXQt");

class b
{
public:
    b() {}

    const QLatin1String defEnv;//("X-LXQT;LXQt");

};

#define SETTINGS_ENTRY(cppname, path, type, defValue) \
type get##cppname() const\
{\
    return mSettings.value(QLatin1String(path), defValue).value<type>();\
}\
void set##cppname(const type& newValue)\
{\
    mSettings.setValue(QLatin1String(path), newValue);\
}

class Settings
{
public:
    Settings();

    // Menu
    SETTINGS_ENTRY(MenuFile, "Menu/menuFile", QString, QLatin1String("/etc/xdg/menus/lxqt-applications.menu"))
    SETTINGS_ENTRY(DesktopEnv, "Menu/desktopEnvironments", QString, QLatin1String("X-LXQT;LXQt"))

    // Buttons
    SETTINGS_ENTRY(ButtonIconSize, "Buttons/icon_size", int, -1)

    // View
    SETTINGS_ENTRY(ViewSpacing,    "View/spacing", int, 0)
    SETTINGS_ENTRY(ViewIconSize,   "View/icon_size", int, -1)

private:
    QSettings mSettings;
};

#undef SETTINGS_ENTRY

#endif // SETTINGS_H
