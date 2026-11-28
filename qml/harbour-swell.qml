// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import "pages"

ApplicationWindow {
    id: app

    // A duration or position in seconds as the Media app shows it
    // (Sailfish.Media MediaListItem.qml:33): Silica's Format, long from an
    // hour on. Empty when unknown.
    function formatTime(secs) {
        if (!(secs >= 0)) {
            return ""
        }
        return Format.formatDuration(secs, secs >= 3600 ? Format.DurationLong : Format.DurationShort)
    }

    // A command to the current player failed, from whichever page sent it.
    Connections {
        target: players.current
        onCommandFailed: {
            console.warn("Command", path, "failed:", error)
            Notices.show(qsTr("The player did not accept that."), Notice.Short)
        }
    }

    ConfigurationValue {
        id: manualHosts
        key: "/io/github/misja/harbour-swell/manualHosts"
        defaultValue: []
    }
    ConfigurationValue {
        id: currentId
        key: "/io/github/misja/harbour-swell/currentId"
        defaultValue: ""
    }
    ConfigurationValue {
        id: currentNodeId
        key: "/io/github/misja/harbour-swell/currentNodeId"
        defaultValue: ""
    }

    Connections {
        target: players
        // First start: when the search ends with exactly one player and none
        // was chosen before, choose it and open its Home.
        onSearchingChanged: {
            if (!players.searching && players.currentId === "" && players.count === 1
                    && pageStack.currentPage && pageStack.currentPage.objectName === "players") {
                players.currentId = players.idAt(0)
                pageStack.currentPage.openCurrent(false)
            }
        }
        onManualHostsChanged: manualHosts.value = players.manualHosts
        onCurrentChanged: {
            currentId.value = players.currentId
            currentNodeId.value = players.currentNodeId
        }
    }

    // The current player on the lock screen and the headset keys (spec 003).
    Mpris {
        player: players.current
        onRaiseRequested: app.activate()
    }

    // Players is the bottom page; with a remembered player the app opens on
    // its Home, one swipe above it.
    initialPage: Component { PlayersPage { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations
    bottomMargin: miniPlayer.visibleSize

    function _homePage() {
        return pageStack.find(function (page) { return page.objectName === "home" })
    }

    // Back to Home in one step from a Browse or Search page (spec 004).
    // Both ignore a tap during a transition: Silica's pop does nothing then
    // (PageStack.js doPop), and Home's openNowPlaying flag would stay set.
    function returnHome() {
        if (pageStack.busy) {
            return
        }
        var home = _homePage()
        if (home) {
            pageStack.pop(home)
        }
    }

    // Opens Now Playing from the mini-player: forward from Home, via Home from
    // Players, back to Home and on from a Browse or Search page, back from the
    // Queue, and from About via the page it was opened from.
    function openNowPlaying() {
        if (pageStack.busy) {
            return
        }
        var page = pageStack.currentPage
        if (page.objectName === "home") {
            pageStack.navigateForward()
        } else if (page.objectName === "browse" || page.objectName === "search"
                   || page.objectName === "presets") {
            var home = _homePage()
            if (home) {
                home.openNowPlaying = true
                pageStack.pop(home)
            }
        } else if (page.objectName === "queue") {
            pageStack.navigateBack()
        } else if (page.objectName === "about") {
            if (pageStack.pop(undefined, PageStackAction.Immediate)) {
                openNowPlaying()
            }
        } else if (page.objectName === "players") {
            page.openCurrent(false)
            var home = pageStack.nextPage(page)
            if (home) {
                home.openNowPlaying = true
            }
        }
    }

    Component.onCompleted: {
        var hosts = manualHosts.value
        for (var i = 0; i < hosts.length; ++i) {
            players.addManual(hosts[i])
        }
        if (currentId.value !== "") {
            players.restoreCurrent(currentId.value, currentNodeId.value)
        }
    }

    // The docked mini-player: on every page except Now Playing, only with a
    // current player that is online and has something to play (hasContent).
    // A drag closes it (DockedPanel sets
    // open = false), so the state is driven by a Binding rather than a plain
    // open: binding. It closes while the stack moves from Home or Players
    // (towards Now Playing), but stays between the other pages: a push makes
    // the new page current before its animation and a pop after it (Silica
    // PageStack.js), so the name stays theirs throughout.
    DockedPanel {
        id: miniPlayer

        readonly property QtObject _player: players.current
        readonly property string _pageName: pageStack.currentPage ? pageStack.currentPage.objectName : ""
        readonly property bool _staysOpen: _pageName === "browse" || _pageName === "search"
                                           || _pageName === "presets"
                                           || _pageName === "queue" || _pageName === "about"

        width: parent.width
        height: pageStack.currentPage && pageStack.currentPage.isLandscape ? Theme.itemSizeMedium : Theme.itemSizeLarge
        dock: Dock.Bottom
        modal: false

        Binding {
            target: miniPlayer
            property: "open"
            value: miniPlayer._player !== null && miniPlayer._player.online
                   && miniPlayer._player.hasContent
                   && (miniPlayer._staysOpen
                       || ((miniPlayer._pageName === "home" || miniPlayer._pageName === "players")
                           && !pageStack.busy))
        }

        BackgroundItem {
            id: miniItem
            anchors.fill: parent
            onClicked: app.openNowPlaying()

            CoverImage {
                id: miniArt
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                width: parent.height - 2 * Theme.paddingMedium
                height: width
                highlighted: miniItem.highlighted
                icon: "image://theme/icon-m-media-songs"
                source: miniPlayer._player ? miniPlayer._player.artworkUrl : ""
            }

            Column {
                anchors {
                    left: miniArt.right
                    leftMargin: Theme.paddingLarge
                    right: playPause.left
                    rightMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    color: miniItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                    text: miniPlayer._player ? miniPlayer._player.title1 : ""
                }
                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: miniItem.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    text: miniPlayer._player ? miniPlayer._player.title2 : ""
                }
            }

            IconButton {
                id: playPause
                anchors {
                    right: parent.right
                    rightMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                icon.source: miniPlayer._player && miniPlayer._player.playing
                             ? "image://theme/icon-m-pause" : "image://theme/icon-m-play"
                onClicked: miniPlayer._player.playing ? miniPlayer._player.pause() : miniPlayer._player.play()
            }
        }
    }
}
