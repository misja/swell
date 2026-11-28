// SPDX-License-Identifier: MIT

#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QScopedPointer>
#include <QStandardPaths>

#include <sailfishapp.h>

#include "covercache.h"
#include "qmlplayer.h"
#include "registration.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setApplicationVersion(QStringLiteral(APP_VERSION));

    Swell::registerTypes();
    Swell::QmlPlayerModel players;

    // Before the view, so it outlives the engine.
    Swell::CoverCacheFactory covers(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                                    + QStringLiteral("/covers"));
    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->engine()->setNetworkAccessManagerFactory(&covers);
    view->rootContext()->setContextProperty(QStringLiteral("players"), &players);
    view->rootContext()->setContextProperty(QStringLiteral("appVersion"), app->applicationVersion());
    view->setSource(SailfishApp::pathToMainQml());
    view->show();

    players.search();
    return app->exec();
}
