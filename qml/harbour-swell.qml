// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import "pages"

ApplicationWindow {
    id: app

    // Shows a short message about a failed command (see HomePage).
    function notify(text) {
        notice.text = text
        noticeTimer.restart()
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
        onManualHostsChanged: manualHosts.value = players.manualHosts
        onCurrentChanged: {
            if (players.currentId !== "") {
                currentId.value = players.currentId
                currentNodeId.value = players.currentNodeId
            }
        }
    }

    // Players is the bottom page; with a remembered player the app opens on
    // its Home, one swipe above it.
    initialPage: Component { PlayersPage { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations
    bottomMargin: miniPlayer.visibleSize

    // Opens Now Playing from the mini-player: forward from Home, or via Home from Players.
    function openNowPlaying() {
        var page = pageStack.currentPage
        if (page.objectName === "home")
            pageStack.navigateForward()
        else if (page.objectName === "players")
            pageStack.push(Qt.resolvedUrl("pages/HomePage.qml"), { openNowPlaying: true })
    }

    Component.onCompleted: {
        var hosts = manualHosts.value
        for (var i = 0; i < hosts.length; ++i)
            players.addManual(hosts[i])
        if (currentId.value !== "") {
            players.restoreCurrent(currentId.value, currentNodeId.value)
            pageStack.push(Qt.resolvedUrl("pages/HomePage.qml"), {}, PageStackAction.Immediate)
        }
    }

    // The docked mini-player: only on Home and Players, only with a current
    // player that is online. A drag closes it (DockedPanel sets open = false),
    // so the state is driven by a Binding rather than a plain open: binding.
    DockedPanel {
        id: miniPlayer

        readonly property QtObject player: players.current
        readonly property string pageName: pageStack.currentPage ? pageStack.currentPage.objectName : ""

        width: parent.width
        height: pageStack.currentPage && pageStack.currentPage.isLandscape ? Theme.itemSizeMedium : Theme.itemSizeLarge
        dock: Dock.Bottom
        modal: false

        Binding {
            target: miniPlayer
            property: "open"
            value: miniPlayer.player !== null && miniPlayer.player.online
                   && (miniPlayer.pageName === "home" || miniPlayer.pageName === "players")
                   && !pageStack.busy
        }

        BackgroundItem {
            anchors.fill: parent
            onClicked: app.openNowPlaying()

            Image {
                id: miniArt
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                width: parent.height - 2 * Theme.paddingMedium
                height: width
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                sourceSize.width: width
                sourceSize.height: height
                source: miniPlayer.player ? miniPlayer.player.artworkUrl : ""
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
                    text: miniPlayer.player ? miniPlayer.player.title1 : ""
                }
                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    text: miniPlayer.player ? miniPlayer.player.title2 : ""
                }
            }

            IconButton {
                id: playPause
                anchors {
                    right: parent.right
                    rightMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                icon.source: miniPlayer.player && miniPlayer.player.playing
                             ? "image://theme/icon-m-pause" : "image://theme/icon-m-play"
                onClicked: miniPlayer.player.playing ? miniPlayer.player.pause() : miniPlayer.player.play()
            }
        }
    }

    Label {
        id: notice
        parent: app.contentItem
        anchors {
            bottom: parent.bottom
            bottomMargin: app.bottomMargin + Theme.paddingLarge
            horizontalCenter: parent.horizontalCenter
        }
        width: parent.width - 2 * Theme.horizontalPageMargin
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: Theme.highlightColor
        opacity: noticeTimer.running ? 1.0 : 0.0
        Behavior on opacity { FadeAnimation {} }
        Timer { id: noticeTimer; interval: 3000 }
    }
}
