// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    readonly property QtObject _player: players.current

    Image {
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        opacity: 0.35
        source: _player ? _player.artworkUrl : ""
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
            text: _player && _player.title1 !== "" ? _player.title1 : qsTr("Swell")
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
            font.pixelSize: Theme.fontSizeSmall
            text: _player ? _player.title2 : ""
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            text: _player ? _player.name : ""
        }
    }

    // Play/pause, plus next song when the source can skip (the queue, or a
    // stream that offers it); plain radio gets play/pause only.
    CoverActionList {
        enabled: _player !== null && _player.online && _player.canSkip

        CoverAction {
            iconSource: _player && _player.playing ? "image://theme/icon-cover-pause" : "image://theme/icon-cover-play"
            onTriggered: _player.playing ? _player.pause() : _player.play()
        }
        CoverAction {
            iconSource: "image://theme/icon-cover-next-song"
            onTriggered: _player.skip()
        }
    }

    CoverActionList {
        enabled: _player !== null && _player.online && !_player.canSkip

        CoverAction {
            iconSource: _player && _player.playing ? "image://theme/icon-cover-pause" : "image://theme/icon-cover-play"
            onTriggered: _player.playing ? _player.pause() : _player.play()
        }
    }
}
