// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0
import Swell 1.0

Page {
    id: page
    objectName: "nowPlaying"

    readonly property QtObject _player: players.current
    // Artwork edge: the short side of the page, flush with the page edges
    // (Silica pitfalls); beside the controls in landscape.
    readonly property real _artSize: isLandscape ? height : width

    allowedOrientations: Orientation.All

    // The queue is the page attached to Now Playing (swipe left).
    onStatusChanged: {
        if (status === PageStatus.Active && pageStack.nextPage(page) === null) {
            pageStack.pushAttached(Qt.resolvedUrl("QueuePage.qml"))
        }
    }

    // The player's place in a group (spec 005, canvas G1), worded as azzurro does.
    function _groupText() {
        if (!_player) {
            return ""
        }
        if (_player.groupRole === Player.Primary) {
            return qsTr("Leading %n player(s)", "", _player.secondaryIds.length)
        }
        if (_player.groupRole === Player.Secondary) {
            return _player.primaryReconnecting ? qsTr("Reconnecting") : qsTr("In another player's group")
        }
        return ""
    }

    function _formatTime(secs) {
        if (secs < 0) {
            return ""
        }
        var m = Math.floor(secs / 60)
        var s = secs % 60
        return m + ":" + (s < 10 ? "0" : "") + s
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: Math.max(height, layout.height + Theme.paddingLarge)

        PullDownMenu {
            MenuItem {
                text: qsTr("Group")
                enabled: _player !== null && _player.online
                onClicked: pageStack.push(Qt.resolvedUrl("GroupPage.qml"))
            }
        }

        Flow {
            id: layout
            width: parent.width
            spacing: Theme.paddingLarge

            Column {
                id: artColumn
                width: page.isLandscape ? page._artSize : page.width

                PageHeader {
                    visible: !page.isLandscape
                    title: _player ? (_player.name !== "" ? _player.name : _player.host) : ""
                    description: _player && !_player.online ? qsTr("Not reachable") : page._groupText()
                }

                Item {
                    width: page._artSize
                    height: page._artSize

                    Image {
                        id: artwork
                        anchors.fill: parent
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        sourceSize.width: width
                        sourceSize.height: height
                        source: _player ? _player.artworkUrl : ""
                    }
                    Icon {
                        anchors.centerIn: parent
                        visible: artwork.status !== Image.Ready
                        source: "image://theme/icon-l-music"
                    }
                }
            }

            Column {
                width: page.isLandscape ? page.width - page._artSize - layout.spacing : page.width
                spacing: Theme.paddingMedium

                PageHeader {
                    id: sideHeader
                    visible: page.isLandscape
                    title: _player ? (_player.name !== "" ? _player.name : _player.host) : ""
                    description: _player && !_player.online ? qsTr("Not reachable") : page._groupText()
                }

                // Centres the controls in the space left beside (landscape) or
                // below (portrait) the artwork, instead of leaving it empty.
                Item {
                    width: parent.width
                    height: Math.max(0, Math.floor((page.height
                                                    - (page.isLandscape ? sideHeader.height : artColumn.height)
                                                    - controls.height) / 2) - Theme.paddingMedium)
                }

                Column {
                    id: controls
                    width: parent.width
                    spacing: Theme.paddingMedium

                // The API requires title1..title3 for three-line now-playing text.
                Column {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin

                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        truncationMode: TruncationMode.Fade
                        font.pixelSize: Theme.fontSizeLarge
                        color: Theme.highlightColor
                        text: _player ? _player.title1 : ""
                    }
                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        truncationMode: TruncationMode.Fade
                        text: _player ? _player.title2 : ""
                    }
                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        truncationMode: TruncationMode.Fade
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                        text: _player ? _player.title3 : ""
                    }
                }

                // Display only: the MVP does not seek.
                Column {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    visible: _player && _player.duration > 0

                    ProgressBar {
                        width: parent.width
                        leftMargin: 0
                        rightMargin: 0
                        minimumValue: 0
                        maximumValue: _player && _player.duration > 0 ? _player.duration : 1
                        value: _player && _player.position > 0 ? _player.position : 0
                    }
                    Item {
                        width: parent.width
                        height: elapsed.height
                        Label {
                            id: elapsed
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryColor
                            text: _player ? page._formatTime(_player.position) : ""
                        }
                        Label {
                            anchors.right: parent.right
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryColor
                            text: _player ? page._formatTime(_player.duration) : ""
                        }
                    }
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: Theme.paddingLarge

                    // Hidden, not greyed, when the source has nothing to go back or
                    // forward to (plain radio: streamUrl without actions, API 4.8).
                    IconButton {
                        anchors.verticalCenter: parent.verticalCenter
                        icon.source: "image://theme/icon-m-previous"
                        visible: _player && _player.canBack
                        enabled: _player && _player.online
                        onClicked: _player.back()
                    }
                    IconButton {
                        anchors.verticalCenter: parent.verticalCenter
                        icon.source: _player && _player.playing ? "image://theme/icon-l-pause" : "image://theme/icon-l-play"
                        enabled: _player && _player.online
                        onClicked: _player.playing ? _player.pause() : _player.play()
                    }
                    IconButton {
                        anchors.verticalCenter: parent.verticalCenter
                        icon.source: "image://theme/icon-m-next"
                        visible: _player && _player.canSkip
                        enabled: _player && _player.online
                        onClicked: _player.skip()
                    }
                }

                Row {
                    x: Theme.paddingMedium
                    width: parent.width - Theme.paddingMedium
                    // Hidden when the player reports a fixed volume (volume=-1).
                    visible: _player && !_player.fixedVolume

                    IconButton {
                        id: mute
                        anchors.verticalCenter: parent.verticalCenter
                        icon.source: _player && _player.muted ? "image://theme/icon-m-speaker-mute" : "image://theme/icon-m-speaker"
                        enabled: _player && _player.online
                        onClicked: _player.setMuted(!_player.muted)
                    }

                    Slider {
                        id: volume
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - mute.width
                        minimumValue: 0
                        maximumValue: 100
                        stepSize: 1
                        enabled: _player && _player.online
                        label: qsTr("Volume")
                        valueText: Math.round(value)

                        // Follow the player, except while the user drags.
                        Binding {
                            target: volume
                            property: "value"
                            value: _player && _player.volume >= 0 ? _player.volume : 0
                            when: !volume.down
                        }
                        onValueChanged: {
                            if (down && _player) {
                                _player.setVolume(Math.round(value))
                            }
                        }
                    }
                }
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
