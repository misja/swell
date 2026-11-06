// SPDX-License-Identifier: MIT

#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QScopedPointer>

#include <sailfishapp.h>

#include "playermodel.h"
#include "registration.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setApplicationVersion(QStringLiteral(APP_VERSION));

    swell::registerTypes();
    bluos::PlayerModel players;

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->rootContext()->setContextProperty(QStringLiteral("players"), &players);
    view->rootContext()->setContextProperty(QStringLiteral("appVersion"), app->applicationVersion());
    view->setSource(SailfishApp::pathToMainQml());
    view->show();

    players.search();
    return app->exec();
}
