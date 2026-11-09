// SPDX-License-Identifier: MIT
// The current player's settings, from Home's pull-down menu (spec 006). Only
// firmware so far: whether the player offers an update, and its own update page.
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    readonly property QtObject _player: players.current
    readonly property bool _updating: _player !== null && _player.availability === 4

    allowedOrientations: Orientation.All

    // Opening the page asks again; so does leaving the update page.
    onStatusChanged: {
        if (status === PageStatus.Active && _player) {
            _player.checkUpgrade()
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: _player ? (_player.name !== "" ? _player.name : _player.host) : ""
                description: qsTr("Settings")
            }

            SectionHeader { text: qsTr("Player") }

            DetailItem {
                label: qsTr("Model")
                value: _player ? _player.modelName : ""
                visible: value !== ""
            }
            DetailItem {
                label: qsTr("Firmware version")
                value: _player ? _player.firmwareVersion : ""
                visible: value !== ""
            }

            SectionHeader { text: qsTr("Firmware") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                text: {
                    if (_updating) {
                        return qsTr("The player is updating. This takes a few minutes.")
                    }
                    if (_player && _player.upgradeAvailable) {
                        return _player.upgradeVersion !== ""
                                ? qsTr("Firmware %1 is available.").arg(_player.upgradeVersion)
                                : qsTr("A firmware update is available.")
                    }
                    // Only once the player has answered a check.
                    return _player && _player.upgradeChecked ? qsTr("No update found.") : ""
                }
                visible: text !== ""
            }

            Item { width: 1; height: Theme.paddingLarge }

            // Only with an update on offer. The player runs it from its own
            // page, as in the official app.
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: _player !== null && _player.upgradeAvailable
                enabled: _player !== null && _player.online
                text: qsTr("Start update")
                onClicked: pageStack.push(Qt.resolvedUrl("PlayerWebPage.qml"), {
                    player: _player,
                    url: _player.upgradePage(),
                    title: _player.name !== "" ? _player.name : _player.host,
                    description: qsTr("Firmware update")
                })
            }
        }

        VerticalScrollDecorator {}
    }
}
