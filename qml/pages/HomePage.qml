// SPDX-License-Identifier: MIT
import QtQuick 2.4   // FontMetrics
import Sailfish.Silica 1.0
import Swell 1.0

Page {
    id: page
    objectName: "home"

    // Set when opened from the mini-player on Players: go straight on to Now Playing.
    property bool openNowPlaying

    readonly property QtObject _player: players.current
    // Read once per change: each read of _player.inputs makes a new array.
    readonly property var _inputs: _player ? _player.inputs : []
    // Columns as in Jolla's Gallery (Sailfish.Gallery ImageGridView.qml):
    // as many as fit at Theme.itemSizeHuge, which scales per device.
    readonly property int _columns: Math.max(1, Math.floor(width / Theme.itemSizeHuge))
    // The quick row (spec 008): two rows in portrait, one in landscape, of
    // the first presets by id. All of them are on the Presets page.
    readonly property var _quickPresets: {
        var list = _player ? _player.presets.slice(0) : []
        list.sort(function(a, b) { return a.id - b.id })
        return list.slice(0, _columns * (isLandscape ? 1 : 2))
    }

    allowedOrientations: Orientation.All

    // The last start sent nothing (Player.upgradeStartRefused).
    property bool _upgradeRefused: false

    Connections {
        target: _player
        onUpgradeStartRefused: page._upgradeRefused = true
    }
    // A refusal belongs to the player it came from.
    on_PlayerChanged: _upgradeRefused = false

    // Only while Home is in front, as Browse does; the app reports failures.
    Connections {
        target: page.status === PageStatus.Active ? page._player : null
        onActionDone: {
            if (type === "preset") {
                Notices.show(qsTr("Added to presets"), Notice.Short)
            }
        }
    }

    // The update notice's first line: the offer, or how a started update goes.
    function _upgradeText() {
        switch (_player.upgradePhase) {
        case Player.UpgradeChecking:
        case Player.UpgradeStarting:
            return qsTr("Starting update")
        case Player.UpgradeInstalling:
            return qsTr("Updating firmware")
        case Player.UpgradeRestarting:
            return qsTr("Restarting")
        case Player.UpgradeNotStarted:
            return qsTr("The update did not start. Restart Swell to try again.")
        case Player.UpgradeUnfinished:
            return qsTr("The update did not finish. Restart Swell to try again.")
        }
        if (_player.availability === Player.Updating) {
            return qsTr("Updating firmware")
        }
        return _upgradeRefused ? qsTr("The update was not started.") : qsTr("Update available")
    }

    onStatusChanged: {
        if (status !== PageStatus.Active) {
            return
        }
        // Now Playing is the page attached to Home (swipe left).
        if (pageStack.nextPage(page) === null) {
            pageStack.pushAttached(Qt.resolvedUrl("NowPlayingPage.qml"))
        }
        if (_player) {
            _player.refreshInputs()
        }
        if (openNowPlaying) {
            openNowPlaying = false
            pageStack.navigateForward()
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        // From the top: the general items, then the current player's, nearest
        // the content (spec 008, decision 11).
        PullDownMenu {
            MenuItem {
                text: qsTr("About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }
            MenuItem {
                text: qsTr("Players")
                onClicked: pageStack.navigateBack()
            }
            MenuItem {
                text: qsTr("Settings")
                enabled: _player !== null
                onClicked: pageStack.push(Qt.resolvedUrl("PlayerSettingsPage.qml"))
            }
            MenuItem {
                text: qsTr("Presets")
                enabled: _player !== null
                onClicked: pageStack.push(Qt.resolvedUrl("PresetsPage.qml"))
            }
            MenuItem {
                text: qsTr("Browse")
                enabled: _player !== null
                onClicked: pageStack.push(Qt.resolvedUrl("BrowsePage.qml"), { root: true })
            }
        }

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: _player ? (_player.name !== "" ? _player.name : _player.host) : ""
                description: {
                    if (!_player) {
                        return qsTr("No player chosen. Pull down to pick one.")
                    }
                    if (_player.online) {
                        return ""
                    }
                    if (_player.availability === Player.Connecting) {
                        return qsTr("Connecting")
                    }
                    if (_player.availability === Player.NotAPlayer) {
                        return qsTr("Not a BluOS player")
                    }
                    if (_player.availability === Player.Updating) {
                        return qsTr("Updating firmware")
                    }
                    return qsTr("Not reachable. Pull down to pick another player.")
                }
            }

            SectionHeader { text: qsTr("Presets") }

            // Silica's grid of items (GridItem.qml), as wide as the page. Not a
            // view of its own here: the page scrolls, so the grid is as tall as
            // its rows. The tiles have no menu (spec 008).
            SilicaGridView {
                id: presetGrid
                width: parent.width
                height: Math.ceil(count / page._columns) * cellHeight
                interactive: false
                visible: count > 0
                cellWidth: Math.floor(width / page._columns)
                cellHeight: cellWidth + nameFont.height + 2 * Theme.paddingSmall
                model: page._quickPresets

                FontMetrics {
                    id: nameFont
                    font.pixelSize: Theme.fontSizeExtraSmall
                }

                delegate: PresetTile {
                    preset: modelData
                    inputs: page._inputs
                    enabled: _player.online
                    onClicked: _player.playPreset(modelData.id)
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: _player && _player.online && page._quickPresets.length === 0
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("No presets on this player. Add them from Browse or from the inputs below.")
            }

            // While the player offers a firmware update, and while one this
            // app started runs.
            BackgroundItem {
                id: updateNotice
                width: parent.width
                height: Math.max(Theme.itemSizeMedium, noticeColumn.height + 2 * Theme.paddingMedium)
                visible: _player !== null && (_player.upgradeAvailable || _player.upgradeStarted
                                              || _player.upgradePhase === Player.UpgradeChecking)
                // Straight to the confirmation (the maintainer, 2026-11-09 and
                // 2026-11-25); only while a start is possible.
                enabled: _player !== null && _player.canStartUpgrade
                onClicked: {
                    page._upgradeRefused = false
                    pageStack.push(Qt.resolvedUrl("UpgradeDialog.qml"), {
                        player: _player,
                        version: _player.upgradeVersion
                    })
                }

                Column {
                    id: noticeColumn
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter

                    Label {
                        width: parent.width
                        wrapMode: Text.Wrap
                        color: updateNotice.highlighted ? Theme.highlightColor : Theme.primaryColor
                        text: _player ? page._upgradeText() : ""
                    }
                    Label {
                        width: parent.width
                        truncationMode: TruncationMode.Fade
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                        visible: text !== ""
                        text: _player && _player.upgradeVersion !== ""
                              ? qsTr("Firmware %1").arg(_player.upgradeVersion) : ""
                    }
                }
            }

            SectionHeader {
                text: qsTr("Inputs")
                visible: inputs.count > 0
            }

            Repeater {
                id: inputs
                model: page._inputs

                ListItem {
                    id: inputItem
                    contentHeight: Theme.itemSizeSmall
                    enabled: _player.online
                    onClicked: _player.playInput(modelData.url)
                    // The player offers no action for an input (spec 008).
                    menu: Component {
                        ContextMenu {
                            MenuItem {
                                text: qsTr("Add preset")
                                onClicked: _player.addInputPreset(modelData.id)
                            }
                        }
                    }

                    Image {
                        id: inputIcon
                        x: Theme.horizontalPageMargin
                        anchors.verticalCenter: parent.verticalCenter
                        width: Theme.iconSizeSmallPlus
                        height: Theme.iconSizeSmallPlus
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
                        color: inputItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                        text: modelData.text
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
