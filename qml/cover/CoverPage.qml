// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    readonly property QtObject player: players.current

    Image {
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        opacity: 0.35
        source: player ? player.artworkUrl : ""
    }

    Column {
        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
            margins: Theme.paddingLarge
        }
        spacing: Theme.paddingSmall

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
            color: Theme.highlightColor
            text: player && player.title1 !== "" ? player.title1 : qsTr("Swell")
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
            font.pixelSize: Theme.fontSizeSmall
            text: player ? player.title2 : ""
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            text: player ? player.name : ""
        }
    }

    // Play/pause, plus next song when the source can skip (the queue, or a
    // stream that offers it); plain radio gets play/pause only.
    CoverActionList {
        enabled: player !== null && player.online && player.canSkip

        CoverAction {
            iconSource: player && player.playing ? "image://theme/icon-cover-pause" : "image://theme/icon-cover-play"
            onTriggered: player.playing ? player.pause() : player.play()
        }
        CoverAction {
            iconSource: "image://theme/icon-cover-next-song"
            onTriggered: player.skip()
        }
    }

    CoverActionList {
        enabled: player !== null && player.online && !player.canSkip

        CoverAction {
            iconSource: player && player.playing ? "image://theme/icon-cover-pause" : "image://theme/icon-cover-play"
            onTriggered: player.playing ? player.pause() : player.play()
        }
    }
}
