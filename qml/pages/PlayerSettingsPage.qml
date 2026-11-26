// SPDX-License-Identifier: MIT
// The current player's settings, from Home's pull-down menu (spec 006). Only
// firmware so far: whether the player offers an update, starting it, and how
// it goes.
import QtQuick 2.0
import Sailfish.Silica 1.0
import Swell 1.0

Page {
    id: page

    readonly property QtObject _player: players.current
    readonly property bool _updating: _player !== null && _player.availability === Player.Updating
    // The last start sent nothing (Player.upgradeStartRefused).
    property bool _refused: false

    allowedOrientations: Orientation.All

    Connections {
        target: _player
        onUpgradeStartRefused: page._refused = true
    }
    // A refusal belongs to the player it came from.
    on_PlayerChanged: _refused = false

    // Opening the page asks again.
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
                    if (!_player) {
                        return ""
                    }
                    switch (_player.upgradePhase) {
                    case Player.UpgradeChecking:
                    case Player.UpgradeStarting:
                        return qsTr("Starting update")
                    case Player.UpgradeInstalling:
                        return qsTr("The player is updating. This takes a few minutes.")
                    case Player.UpgradeRestarting:
                        return qsTr("Restarting")
                    case Player.UpgradeDone:
                        return qsTr("Updated to %1").arg(_player.firmwareVersion)
                    case Player.UpgradeNotStarted:
                        return qsTr("The update did not start. Restart Swell to try again.")
                    case Player.UpgradeUnfinished:
                        return qsTr("The update did not finish. Restart Swell to try again.")
                    }
                    if (_updating) {
                        return qsTr("The player is updating. This takes a few minutes.")
                    }
                    if (page._refused) {
                        return qsTr("The update was not started.")
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

            // Only with an update on offer, and not once a start was sent;
            // disabled while a start is not possible (offline, say).
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: _player !== null && _player.upgradeAvailable && !_updating
                         && !_player.upgradeStarted && _player.upgradePhase !== Player.UpgradeChecking
                enabled: _player !== null && _player.canStartUpgrade
                text: qsTr("Start update")
                onClicked: {
                    page._refused = false
                    pageStack.push(Qt.resolvedUrl("UpgradeDialog.qml"), {
                        player: _player,
                        version: _player.upgradeVersion
                    })
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
