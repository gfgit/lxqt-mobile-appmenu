#include "settings.h"

#include <QDebug>

Settings::Settings() :
    mSettings(QSettings::UserScope)
{
    qDebug() << "SETTINGS:" << mSettings.fileName();
}
