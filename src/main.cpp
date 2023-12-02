#include "appmenuwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication::setOrganizationDomain(QLatin1String("org.gfgit"));
    QApplication::setApplicationName(QLatin1String("lxqt-mobile-appmenu"));
    QApplication::setApplicationDisplayName(QLatin1String("Mobile App Menu (LXQt)"));
    QApplication::setApplicationVersion(QLatin1String("1.0"));

    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(true);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "lxqt-mobile-appmenu_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    AppMenuWindow w;
    w.loadSettings();
    w.showMaximized();
    return a.exec();
}
