// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    objectName: "nowPlaying"

    property QtObject player: players.current
    // Artwork edge: the short side of the page, less margins; beside the
    // controls in landscape.
    readonly property real artSize: isLandscape
                                    ? height - 2 * Theme.paddingLarge
                                    : width - 2 * Theme.horizontalPageMargin

    allowedOrientations: Orientation.All

    function formatTime(secs) {
        if (secs < 0)
            return ""
        var m = Math.floor(secs / 60)
        var s = secs % 60
        return m + ":" + (s < 10 ? "0" : "") + s
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: Math.max(height, layout.height + Theme.paddingLarge)

        Flow {
            id: layout
            width: parent.width
            y: page.isLandscape ? Theme.paddingLarge : 0
            spacing: Theme.paddingLarge

            Column {
                width: page.isLandscape ? page.artSize + Theme.horizontalPageMargin : page.width

                PageHeader {
                    visible: !page.isLandscape
                    title: player ? (player.name !== "" ? player.name : player.host) : ""
                    description: player && !player.online ? qsTr("Not reachable") : ""
                }

                Item {
                    x: Theme.horizontalPageMargin
                    width: page.artSize
                    height: page.artSize

                    Image {
                        id: artwork
                        anchors.fill: parent
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        sourceSize.width: width
                        sourceSize.height: height
                        source: player ? player.artworkUrl : ""
                    }
                    Icon {
                        anchors.centerIn: parent
                        visible: artwork.status !== Image.Ready
                        source: "image://theme/icon-l-music"
                    }
                }
            }

            Column {
                width: page.isLandscape ? page.width - page.artSize - 2 * Theme.horizontalPageMargin - layout.spacing
                                        : page.width
                spacing: Theme.paddingMedium

                PageHeader {
                    visible: page.isLandscape
                    title: player ? (player.name !== "" ? player.name : player.host) : ""
                    description: player && !player.online ? qsTr("Not reachable") : ""
                }

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
                        text: player ? player.title1 : ""
                    }
                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        truncationMode: TruncationMode.Fade
                        text: player ? player.title2 : ""
                    }
                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        truncationMode: TruncationMode.Fade
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                        text: player ? player.title3 : ""
                    }
                }

                // Display only: the MVP does not seek.
                Column {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    visible: player && player.duration > 0

                    ProgressBar {
                        width: parent.width
                        leftMargin: 0
                        rightMargin: 0
                        minimumValue: 0
                        maximumValue: player && player.duration > 0 ? player.duration : 1
                        value: player && player.position > 0 ? player.position : 0
                    }
                    Item {
                        width: parent.width
                        height: elapsed.height
                        Label {
                            id: elapsed
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryColor
                            text: player ? page.formatTime(player.position) : ""
                        }
                        Label {
                            anchors.right: parent.right
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryColor
                            text: player ? page.formatTime(player.duration) : ""
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
                        visible: player && player.canBack
                        enabled: player && player.online
                        onClicked: player.back()
                    }
                    IconButton {
                        anchors.verticalCenter: parent.verticalCenter
                        icon.source: player && player.playing ? "image://theme/icon-l-pause" : "image://theme/icon-l-play"
                        enabled: player && player.online
                        onClicked: player.playing ? player.pause() : player.play()
                    }
                    IconButton {
                        anchors.verticalCenter: parent.verticalCenter
                        icon.source: "image://theme/icon-m-next"
                        visible: player && player.canSkip
                        enabled: player && player.online
                        onClicked: player.skip()
                    }
                }

                Row {
                    x: Theme.paddingMedium
                    width: parent.width - Theme.paddingMedium
                    // Hidden when the player reports a fixed volume (volume=-1).
                    visible: player && !player.fixedVolume

                    IconButton {
                        id: mute
                        anchors.verticalCenter: parent.verticalCenter
                        icon.source: player && player.muted ? "image://theme/icon-m-speaker-mute" : "image://theme/icon-m-speaker"
                        enabled: player && player.online
                        onClicked: player.setMuted(!player.muted)
                    }

                    Slider {
                        id: volume
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - mute.width
                        minimumValue: 0
                        maximumValue: 100
                        stepSize: 1
                        enabled: player && player.online
                        label: qsTr("Volume")
                        valueText: Math.round(value)

                        // Follow the player, except while the user drags.
                        Binding {
                            target: volume
                            property: "value"
                            value: player && player.volume >= 0 ? player.volume : 0
                            when: !volume.down
                        }
                        onValueChanged: if (down && player) player.setVolume(Math.round(value))
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
