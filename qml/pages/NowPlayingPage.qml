// SPDX-License-Identifier: MIT
import QtQuick 2.0
import QtQuick.Layouts 1.1   // ColumnLayout
import Sailfish.Silica 1.0
import Swell 1.0

Page {
    id: page
    objectName: "nowPlaying"

    readonly property QtObject _player: players.current
    // A leader: the slider is the group volume, with the members' own
    // volumes folding out under it (spec 005, canvas A2).
    readonly property bool _leads: _player !== null && _player.groupRole === Player.Primary
    property bool _perPlayer: false
    // Nothing to play (an emptied queue): a label instead of empty artwork,
    // titles and transport; volume and Per player stay.
    readonly property bool _empty: _player !== null && _player.online && !_player.hasContent
    // Shuffle and repeat only while the queue is the source (API 2.1, streamUrl).
    readonly property bool _queueModes: !_empty && _player !== null && !_player.streaming
    // Artwork edge: the short side of the page, flush with the page edges
    // (Silica pitfalls); beside the controls in landscape.
    readonly property real _artSize: isLandscape ? height : width

    allowedOrientations: Orientation.All

    // Members keep their own volume current only while it is shown.
    Binding {
        target: players
        property: "watchGroup"
        value: page.status === PageStatus.Active && Qt.application.active && page._leads && page._perPlayer
    }
    Component.onDestruction: players.watchGroup = false

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

    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        contentHeight: Math.max(height, layout.height + Theme.paddingLarge)

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
                    visible: !page._empty
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

            // The controls centred in the space beside (landscape) or below
            // (portrait) the artwork: a layout filling that space, with
            // room taking the rest above and below them (Layout.fillHeight).
            ColumnLayout {
                width: page.isLandscape ? page.width - page._artSize - layout.spacing : page.width
                // The page's height less what is above (artwork and the Flow's
                // spacing in portrait) and the flickable's bottom padding.
                height: Math.max(implicitHeight, page.height - Theme.paddingLarge
                                 - (page.isLandscape ? 0 : artColumn.height + layout.spacing))
                spacing: Theme.paddingMedium

                PageHeader {
                    id: sideHeader
                    Layout.fillWidth: true
                    visible: page.isLandscape
                    title: _player ? (_player.name !== "" ? _player.name : _player.host) : ""
                    description: _player && !_player.online ? qsTr("Not reachable") : page._groupText()
                }

                Item {
                    Layout.fillHeight: true
                }

                Column {
                    id: controls
                    Layout.fillWidth: true
                    spacing: Theme.paddingMedium

                    InfoLabel {
                        visible: page._empty
                        text: qsTr("Nothing to play")
                    }

                    // The API requires title1..title3 for three-line now-playing text.
                    Column {
                        visible: !page._empty
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

                    // The position; a seek slider when the track can be moved
                    // within (as the Media app's, MediaPlayerControlsPanel.qml:112),
                    // a bar otherwise (spec 001, #73).
                    Column {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        visible: _player && _player.duration > 0

                        Slider {
                            id: seekSlider
                            visible: _player !== null && _player.seekable
                            width: parent.width
                            leftMargin: 0
                            rightMargin: 0
                            handleVisible: false
                            minimumValue: 0
                            maximumValue: _player && _player.duration > 0 ? _player.duration : 1
                            enabled: _player !== null && _player.online
                            // Follow the player, except while the finger is on it.
                            Binding {
                                target: seekSlider
                                property: "value"
                                value: _player && _player.position > 0 ? _player.position : 0
                                when: !seekSlider.down
                            }
                            // Once, on release (a cancelled drag sends nothing).
                            onReleased: _player.seek(Math.round(value))
                        }
                        ProgressBar {
                            visible: !seekSlider.visible
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
                                // In the total's format, so both labels match; the
                                // finger's place while seeking.
                                text: _player ? app.formatTime(seekSlider.down ? Math.round(seekSlider.value) : _player.position,
                                                               _player.duration) : ""
                            }
                            Label {
                                anchors.right: parent.right
                                font.pixelSize: Theme.fontSizeExtraSmall
                                color: Theme.secondaryColor
                                text: _player ? app.formatTime(_player.duration) : ""
                            }
                        }
                    }

                    // The transport, with shuffle and repeat as the platform's
                    // media controls show them (Sailfish.Media
                    // MediaPlayerControlsPanel: icon Switches that follow the
                    // state), only while the queue is the source (API 2.1). One
                    // grid: in portrait the Switches sit in a second row under
                    // previous and next; in landscape at both ends of the first,
                    // so the volume stays in view.
                    GridLayout {
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible: !page._empty
                        columns: 5
                        columnSpacing: Theme.paddingLarge
                        rowSpacing: Theme.paddingMedium

                        Switch {
                            objectName: "shuffle"
                            Layout.row: page.isLandscape ? 0 : 1
                            Layout.column: page.isLandscape ? 0 : 1
                            Layout.alignment: Qt.AlignCenter
                            visible: page._queueModes
                            icon.source: "image://theme/icon-m-shuffle"
                            automaticCheck: false
                            checked: _player !== null && _player.shuffle
                            enabled: _player !== null && _player.online
                            onClicked: _player.setShuffle(!_player.shuffle)
                        }
                        // Hidden, not greyed, when the source has nothing to go back or
                        // forward to (plain radio: streamUrl without actions, API 4.8).
                        IconButton {
                            Layout.row: 0
                            Layout.column: 1
                            Layout.alignment: Qt.AlignCenter
                            icon.source: "image://theme/icon-m-previous"
                            visible: _player && _player.canBack
                            enabled: _player && _player.online
                            onClicked: _player.back()
                        }
                        IconButton {
                            Layout.row: 0
                            Layout.column: 2
                            Layout.alignment: Qt.AlignCenter
                            icon.source: _player && _player.playing ? "image://theme/icon-l-pause" : "image://theme/icon-l-play"
                            enabled: _player && _player.online
                            onClicked: _player.playing ? _player.pause() : _player.play()
                        }
                        IconButton {
                            Layout.row: 0
                            Layout.column: 3
                            Layout.alignment: Qt.AlignCenter
                            icon.source: "image://theme/icon-m-next"
                            visible: _player && _player.canSkip
                            enabled: _player && _player.online
                            onClicked: _player.skip()
                        }
                        Switch {
                            objectName: "repeat"
                            Layout.row: page.isLandscape ? 0 : 1
                            Layout.column: page.isLandscape ? 4 : 3
                            Layout.alignment: Qt.AlignCenter
                            visible: page._queueModes
                            icon.source: _player !== null && _player.repeat === Player.RepeatTrack
                                         ? "image://theme/icon-m-repeat-single" : "image://theme/icon-m-repeat"
                            automaticCheck: false
                            checked: _player !== null && _player.repeat !== Player.RepeatOff
                            enabled: _player !== null && _player.online
                            // Off, then the whole queue, then the track.
                            onClicked: {
                                switch (_player.repeat) {
                                case Player.RepeatOff:
                                    _player.setRepeat(Player.RepeatQueue)
                                    break
                                case Player.RepeatQueue:
                                    _player.setRepeat(Player.RepeatTrack)
                                    break
                                default:
                                    _player.setRepeat(Player.RepeatOff)
                                }
                            }
                        }
                    }

                    Row {
                        x: Theme.paddingMedium
                        width: parent.width - Theme.paddingMedium
                        // Hidden when the player reports a fixed volume (volume=-1),
                        // unless it leads a group.
                        visible: _player && (page._leads ? (_player.groupVolume >= 0 || !_player.fixedVolume)
                                                         : !_player.fixedVolume)

                        IconButton {
                            id: mute
                            anchors.verticalCenter: parent.verticalCenter
                            icon.source: (page._leads ? players.currentGroupMuted : _player && _player.muted)
                                         ? "image://theme/icon-m-speaker-mute" : "image://theme/icon-m-speaker"
                            enabled: _player && _player.online
                            // A leader mutes the whole group, like its slider.
                            onClicked: {
                                if (page._leads) {
                                    _player.setGroupMuted(!players.currentGroupMuted)
                                } else {
                                    _player.setMuted(!_player.muted)
                                }
                            }
                        }

                        Slider {
                            id: volume
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width - mute.width
                            minimumValue: 0
                            maximumValue: 100
                            stepSize: 1
                            enabled: _player && _player.online
                            label: page._leads ? qsTr("Group volume") : qsTr("Volume")
                            valueText: Math.round(value)

                            // Follow the player, except while the user drags.
                            Binding {
                                target: volume
                                property: "value"
                                value: {
                                    if (!_player) {
                                        return 0
                                    }
                                    if (page._leads && _player.groupVolume >= 0) {
                                        return _player.groupVolume
                                    }
                                    return Math.max(0, _player.volume)
                                }
                                when: !volume.down
                            }
                            onValueChanged: {
                                if (down && _player) {
                                    if (page._leads) {
                                        _player.setGroupVolume(Math.round(value))
                                    } else {
                                        _player.setVolume(Math.round(value))
                                    }
                                }
                            }
                        }
                    }

                    // Silica's own fold-out; members are watched only while open.
                    // The group scrolls the opened section into view
                    // (ExpandingSectionGroup.qml, animateToExpandedSection).
                    ExpandingSectionGroup {
                        visible: page._leads

                        ExpandingSection {
                            id: perPlayer
                            title: qsTr("Per player")
                            onExpandedChanged: page._perPlayer = expanded

                            content.sourceComponent: Column {
                                width: perPlayer.width
                                Repeater {
                                    model: CurrentGroup {
                                        sourceModel: players
                                    }
                                    delegate: Row {
                                        x: Theme.paddingMedium
                                        width: parent.width - Theme.paddingMedium

                                        // This member's own mute.
                                        IconButton {
                                            width: mute.width
                                            height: Theme.itemSizeSmall
                                            anchors.verticalCenter: parent.verticalCenter
                                            // The theme's small volume icons, drawn for this size.
                                            icon.source: model.muted ? "image://theme/icon-system-volume-mute" : "image://theme/icon-system-volume"
                                            enabled: model.online
                                            onClicked: players.setPlayerMuted(model.playerId, !model.muted)
                                        }

                                        // A member with a fixed volume: its name only.
                                        Label {
                                            visible: model.volume < 0
                                            width: parent.width - mute.width
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: qsTr("%1: fixed volume").arg(model.name)
                                            color: Theme.secondaryColor
                                        }
                                        Slider {
                                            id: member
                                            visible: model.volume >= 0
                                            width: parent.width - mute.width
                                            enabled: model.online
                                            minimumValue: 0
                                            maximumValue: 100
                                            stepSize: 1
                                            label: model.name
                                            valueText: Math.round(value)

                                            Binding {
                                                target: member
                                                property: "value"
                                                value: Math.max(0, model.volume)
                                                when: !member.down
                                            }
                                            onValueChanged: {
                                                if (down) {
                                                    players.setPlayerVolume(model.playerId, Math.round(value))
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
