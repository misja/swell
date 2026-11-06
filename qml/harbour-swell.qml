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

    Component.onCompleted: {
        var hosts = manualHosts.value
        for (var i = 0; i < hosts.length; ++i)
            players.addManual(hosts[i])
        if (currentId.value !== "") {
            players.restoreCurrent(currentId.value, currentNodeId.value)
            pageStack.push(Qt.resolvedUrl("pages/HomePage.qml"), {}, PageStackAction.Immediate)
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
