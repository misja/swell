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
    readonly property var _presets: _player ? _player.presets : []
    readonly property int _columns: isLandscape ? 6 : 4
    readonly property int _tileSize: Math.floor((width - 2 * Theme.horizontalPageMargin
                                                - (_columns - 1) * Theme.paddingMedium) / _columns)

    allowedOrientations: Orientation.All

    // The last start sent nothing (Player.upgradeStartRefused).
    property bool _upgradeRefused: false

    Connections {
        target: _player
        onUpgradeStartRefused: page._upgradeRefused = true
    }
    // A refusal belongs to the player it came from.
    on_PlayerChanged: _upgradeRefused = false

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

    // Delete a preset after a remorse (spec 008). The remorse is the page's,
    // not the tile's: a presets change recreates the tiles. It keeps the
    // player and the presets' version the user saw.
    function _deletePreset(id, name) {
        var player = _player
        var prid = player.prid
        deleteRemorse.execute(qsTr("Deleting %1").arg(name), function() {
            if (players.current !== player) {
                app.notify(qsTr("Nothing was deleted."))
                return
            }
            player.deletePreset(id, prid)
        })
    }

    // The icon the user chose for an input in the official app; presets that
    // select an input carry only a generic one.
    function _inputImage(inputId) {
        if (!_player || inputId === "") {
            return ""
        }
        for (var i = 0; i < _inputs.length; ++i) {
            if (_inputs[i].id === inputId) {
                return _inputs[i].image
            }
        }
        return ""
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

    Connections {
        target: _player
        onPresetDeleteRefused: app.notify(presetsChanged ? qsTr("The presets have changed. Nothing was deleted.")
                                                         : qsTr("Nothing was deleted."))
    }

    RemorsePopup { id: deleteRemorse }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }
            MenuItem {
                text: qsTr("Settings")
                enabled: _player !== null
                onClicked: pageStack.push(Qt.resolvedUrl("PlayerSettingsPage.qml"))
            }
            MenuItem {
                text: qsTr("Players")
                onClicked: pageStack.navigateBack()
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

            // Silica's grid of items with a context menu (GridItem.qml). Not
            // a view of its own here: the page scrolls, so the grid is as tall
            // as its rows plus an open menu (SilicaGridView.qml:59 does not
            // add __silica_menu_height to contentHeight).
            SilicaGridView {
                id: presetGrid
                readonly property int _rows: Math.ceil(count / page._columns)
                x: Theme.horizontalPageMargin
                // The last column's spacing lies outside the page margin.
                width: parent.width - 2 * Theme.horizontalPageMargin + Theme.paddingMedium
                height: _rows * cellHeight + __silica_menu_height
                interactive: false
                visible: count > 0
                cellWidth: page._tileSize + Theme.paddingMedium
                cellHeight: page._tileSize + nameFont.height + Theme.paddingSmall + Theme.paddingMedium
                model: page._presets

                FontMetrics {
                    id: nameFont
                    font.pixelSize: Theme.fontSizeExtraSmall
                }

                delegate: GridItem {
                    id: tile
                    readonly property bool _isInput: modelData.inputId !== ""
                    width: page._tileSize
                    contentHeight: presetGrid.cellHeight - Theme.paddingMedium
                    enabled: _player.online
                    onClicked: _player.playPreset(modelData.id)
                    menu: Component {
                        ContextMenu {
                            MenuItem {
                                text: qsTr("Delete")
                                onClicked: page._deletePreset(modelData.id, modelData.name)
                            }
                        }
                    }

                    // Inputs are line icons: give them a tile like the station logos.
                    Rectangle {
                        width: page._tileSize
                        height: page._tileSize
                        visible: tile._isInput
                        color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
                    }
                    Image {
                        id: inputArt
                        visible: tile._isInput
                        x: (page._tileSize - width) / 2
                        y: (page._tileSize - height) / 2
                        width: page._tileSize * 0.55
                        height: width
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        sourceSize.width: width
                        sourceSize.height: height
                        source: tile._isInput ? page._inputImage(modelData.inputId) || modelData.image : ""
                    }
                    Icon {
                        x: (page._tileSize - width) / 2
                        y: (page._tileSize - height) / 2
                        visible: tile._isInput && inputArt.status !== Image.Ready
                        source: "image://theme/icon-m-media-radio"
                    }
                    // Stations, albums and playlists: the cover placeholder.
                    CoverImage {
                        visible: !tile._isInput
                        width: page._tileSize
                        height: width
                        highlighted: tile.highlighted
                        icon: "image://theme/icon-m-media-radio"
                        source: tile._isInput ? "" : modelData.image
                    }
                    Label {
                        id: nameLabel
                        y: page._tileSize
                        anchors.topMargin: Theme.paddingSmall
                        width: parent.width
                        font.pixelSize: Theme.fontSizeExtraSmall
                        truncationMode: TruncationMode.Fade
                        color: tile.highlighted ? Theme.highlightColor : Theme.primaryColor
                        text: modelData.name
                    }
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: _player && _player.online && _player.presets.length === 0
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("No presets on this player. Presets are made in the BluOS Controller app.")
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
