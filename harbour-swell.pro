# SPDX-License-Identifier: MIT
# Based on the Sailfish SDK 3.13.5 sailfishos-qtquick2app (qmake) template.

TARGET = harbour-swell

CONFIG += sailfishapp
QT += network

VERSION = 0.5.0
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

include(bluos/bluos.pri)

INCLUDEPATH += src
HEADERS += src/registration.h src/qmlplayer.h src/qmlbrowsemodel.h src/playerlistmodel.h
SOURCES += src/harbour-swell.cpp src/registration.cpp src/qmlplayer.cpp src/qmlbrowsemodel.cpp src/playerlistmodel.cpp

DISTFILES += qml/harbour-swell.qml \
    qml/Mpris.qml \
    qml/cover/CoverPage.qml \
    qml/pages/PlayersPage.qml \
    qml/pages/AddPlayerDialog.qml \
    qml/pages/HomePage.qml \
    qml/pages/NowPlayingPage.qml \
    qml/pages/QueuePage.qml \
    qml/pages/BrowsePage.qml \
    qml/pages/LetterIndex.qml \
    qml/pages/CoverImage.qml \
    qml/pages/SearchPage.qml \
    qml/pages/AboutPage.qml \
    rpm/harbour-swell.changes.run \
    rpm/harbour-swell.spec \
    translations/*.ts \
    harbour-swell.desktop

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

CONFIG += sailfishapp_i18n

TRANSLATIONS += translations/harbour-swell-nl.ts \
    translations/harbour-swell-en.ts

# Harbour allows files only below /usr/share/$$TARGET, so the license text
# is installed there rather than with %license.
license.files = LICENSE
license.path = /usr/share/$${TARGET}
INSTALLS += license
