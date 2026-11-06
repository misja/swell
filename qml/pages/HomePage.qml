// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property QtObject player: players.current
    readonly property int tileSize: (width - 2 * Theme.horizontalPageMargin - 3 * Theme.paddingMedium) / (isLandscape ? 6 : 4)

    allowedOrientations: Orientation.All

    onStatusChanged: if (status === PageStatus.Active && player) player.refreshInputs()

    Connections {
        target: player
        onCommandFailed: app.notify(qsTr("The player did not accept that: %1").arg(error))
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }
            MenuItem {
                text: qsTr("Players")
                onClicked: pageStack.navigateBack()
            }
        }

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: player ? (player.name !== "" ? player.name : player.host) : ""
                description: {
                    if (!player || player.online)
                        return ""
                    if (player.availability === 0)
                        return qsTr("Connecting…")
                    if (player.availability === 3)
                        return qsTr("Not a BluOS player")
                    return qsTr("Not reachable. Pull down to pick another player.")
                }
            }

            SectionHeader { text: qsTr("Presets") }

            Flow {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingMedium
                visible: player && player.presets.length > 0

                Repeater {
                    model: player ? player.presets : []

                    BackgroundItem {
                        width: page.tileSize
                        height: page.tileSize + nameLabel.height + Theme.paddingSmall
                        enabled: player.online
                        onClicked: player.playPreset(modelData.id)

                        Image {
                            id: art
                            width: page.tileSize
                            height: page.tileSize
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            sourceSize.width: width
                            sourceSize.height: height
                            source: modelData.image
                        }
                        Icon {
                            anchors.centerIn: art
                            visible: art.status !== Image.Ready
                            source: "image://theme/icon-m-media-radio"
                        }
                        Label {
                            id: nameLabel
                            anchors.top: art.bottom
                            anchors.topMargin: Theme.paddingSmall
                            width: parent.width
                            font.pixelSize: Theme.fontSizeExtraSmall
                            truncationMode: TruncationMode.Fade
                            text: modelData.name
                        }
                    }
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: player && player.online && player.presets.length === 0
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("No presets on this player. Presets are created in the official app.")
            }

            SectionHeader {
                text: qsTr("Inputs")
                visible: inputs.count > 0
            }

            Repeater {
                id: inputs
                model: player ? player.inputs : []

                ListItem {
                    contentHeight: Theme.itemSizeSmall
                    enabled: player.online
                    onClicked: player.playInput(modelData.url)

                    Image {
                        id: inputIcon
                        x: Theme.horizontalPageMargin
                        anchors.verticalCenter: parent.verticalCenter
                        width: Theme.iconSizeMedium
                        height: Theme.iconSizeMedium
                        fillMode: Image.PreserveAspectFit
                        sourceSize.width: width
                        sourceSize.height: height
                        source: modelData.image
                    }
                    Label {
                        anchors {
                            left: inputIcon.right
                            leftMargin: Theme.paddingLarge
                            right: parent.right
                            rightMargin: Theme.horizontalPageMargin
                            verticalCenter: parent.verticalCenter
                        }
                        truncationMode: TruncationMode.Fade
                        text: modelData.text
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
