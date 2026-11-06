// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property QtObject player: app.currentPlayer

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
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: player ? player.name || player.host : ""
                description: player && !player.online
                             ? (player.availability === 3 ? qsTr("Not a BluOS player") : qsTr("Not reachable"))
                             : ""
            }

            Item {
                width: parent.width
                height: Math.min(page.width, page.height) - 4 * Theme.horizontalPageMargin

                Image {
                    id: artwork
                    anchors.centerIn: parent
                    width: parent.height
                    height: parent.height
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    source: player ? player.artworkUrl : ""
                }

                Icon {
                    anchors.centerIn: parent
                    visible: artwork.status !== Image.Ready
                    source: "image://theme/icon-l-music"
                }
            }

            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin

                // The API requires title1..title3 for three-line now-playing text.
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
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    text: player ? player.title3 : ""
                }
            }

            Slider {
                width: parent.width
                visible: player && player.duration > 0
                enabled: false
                minimumValue: 0
                maximumValue: player && player.duration > 0 ? player.duration : 1
                value: player && player.position > 0 ? player.position : 0
                valueText: player ? formatTime(player.position) + " / " + formatTime(player.duration) : ""
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.paddingLarge

                IconButton {
                    icon.source: "image://theme/icon-m-previous"
                    enabled: player && player.online && player.canSkip
                    onClicked: player.back()
                }
                IconButton {
                    icon.source: player && player.playing ? "image://theme/icon-l-pause" : "image://theme/icon-l-play"
                    enabled: player && player.online
                    onClicked: player.playing ? player.pause() : player.play()
                }
                IconButton {
                    icon.source: "image://theme/icon-m-next"
                    enabled: player && player.online && player.canSkip
                    onClicked: player.skip()
                }
            }

            Row {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
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

                    // Follow the player except while the user is dragging.
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

        VerticalScrollDecorator {}
    }
}
