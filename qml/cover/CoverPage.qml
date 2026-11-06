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
            truncationMode: TruncationMode.Fade
            color: Theme.highlightColor
            text: player && player.title1 !== "" ? player.title1 : qsTr("Swell")
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeSmall
            text: player ? player.title2 : ""
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            text: player ? player.name : ""
        }
    }

    // Two cover actions, the platform maximum: play/pause and next.
    CoverActionList {
        enabled: player !== null && player.online

        CoverAction {
            iconSource: player && player.playing ? "image://theme/icon-cover-pause" : "image://theme/icon-cover-play"
            onTriggered: player.playing ? player.pause() : player.play()
        }
        CoverAction {
            iconSource: "image://theme/icon-cover-next-song"
            onTriggered: player.skip()
        }
    }
}
