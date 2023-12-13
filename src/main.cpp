#include "appmenuwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QLocale>
#include <QTranslator>

#include <QLocalSocket>
#include <QLocalServer>

#include <QDebug>

const QLatin1String localServerName("lxqt-mobile-appmenu-1.0");

int main(int argc, char *argv[])
{
    QApplication::setOrganizationDomain(QLatin1String("org.gfgit"));
    QApplication::setApplicationName(QLatin1String("lxqt-mobile-appmenu"));
    QApplication::setApplicationDisplayName(QLatin1String("Mobile App Menu (LXQt)"));
    QApplication::setApplicationVersion(QLatin1String("1.0"));

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(true);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "lxqt-mobile-appmenu_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            app.installTranslator(&translator);
            break;
        }
    }

    QCommandLineOption toggleOpt("toggle",
                                 "Try to connect to a running instance started with this option.\n"
                                 "If found it request to close it and then quits.\n"
                                 "If no instance is found then it open a new window\n"
                                 "and start listening for close requests.");

    QCommandLineParser parser;
    parser.addOption(toggleOpt);

    parser.process(app);

    QScopedPointer<QLocalServer> server;
    QScopedPointer<QLocalSocket> appSocket;

    if(parser.isSet(toggleOpt))
    {
        // Try to find running server
        appSocket.reset(new QLocalSocket);

        appSocket->connectToServer(localServerName);
        if(appSocket->waitForConnected())
        {
            qDebug() << "Connected to:" << appSocket->fullServerName() << "Requesting to quit it.";
            appSocket->write("!"); // Random data, maybe not needed
            appSocket->waitForBytesWritten();
            appSocket->close();
            return 0;
        }
        appSocket.reset();

        // We are first instance, launch server
        server.reset(new QLocalServer);
        if(!server->listen(localServerName))
        {
            qWarning() << "Could not register server:" << localServerName << server->errorString();
            return 0;
        }

        qDebug() << "Setting up server:" << server->fullServerName();

        // Listen to close requests
        QObject::connect(server.get(), &QLocalServer::newConnection, server.get(), [&server]()
                         {
                             QLocalSocket *conn = server->nextPendingConnection();
                             qDebug() << "Received connection from:" << conn->fullServerName() << conn->socketDescriptor();
                             conn->close();
                             QCoreApplication::quit();
                             qDebug() << "Quit requested...";
                         });
    }

    AppMenuWindow w;
    w.loadSettings();
    w.resetUi();
    w.showMaximized();

    return app.exec();
}
