// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    objectName: "home"

    // Set when opened from the mini-player on Players: go straight on to Now Playing.
    property bool openNowPlaying

    readonly property QtObject _player: players.current
    readonly property int _columns: isLandscape ? 6 : 4
    readonly property int _tileSize: Math.floor((width - 2 * Theme.horizontalPageMargin
                                                - (_columns - 1) * Theme.paddingMedium) / _columns)

    allowedOrientations: Orientation.All

    // The icon the user chose for an input in the official app; presets that
    // select an input carry only a generic one.
    function _inputImage(inputId) {
        if (!_player || inputId === "") {
            return ""
        }
        for (var i = 0; i < _player.inputs.length; ++i) {
            if (_player.inputs[i].id === inputId) {
                return _player.inputs[i].image
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
        onCommandFailed: {
            console.warn("Command", path, "failed:", error)
            app.notify(qsTr("The player did not accept that."))
        }
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
                    if (_player.availability === 0) {
                        return qsTr("Connecting")
                    }
                    if (_player.availability === 3) {
                        return qsTr("Not a BluOS player")
                    }
                    if (_player.availability === 4) {
                        return qsTr("Updating firmware")
                    }
                    return qsTr("Not reachable. Pull down to pick another player.")
                }
            }

            SectionHeader { text: qsTr("Presets") }

            Flow {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingMedium
                visible: _player && _player.presets.length > 0

                Repeater {
                    model: _player ? _player.presets : []

                    BackgroundItem {
                        id: tile
                        readonly property bool _isInput: modelData.inputId !== ""
                        width: page._tileSize
                        height: page._tileSize + nameLabel.height + Theme.paddingSmall
                        enabled: _player.online
                        onClicked: _player.playPreset(modelData.id)

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

            // Only while the player offers a firmware update.
            BackgroundItem {
                id: updateNotice
                width: parent.width
                height: Theme.itemSizeMedium
                visible: _player !== null && _player.upgradeAvailable
                // The page is the player's own: only while it answers.
                enabled: _player !== null && _player.online
                // Straight to the player's own update page (the maintainer, 2026-11-09).
                onClicked: pageStack.push(Qt.resolvedUrl("PlayerWebPage.qml"), {
                    player: _player,
                    url: _player.upgradePage(),
                    title: _player.name !== "" ? _player.name : _player.host,
                    description: qsTr("Firmware update")
                })

                Column {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter

                    Label {
                        width: parent.width
                        truncationMode: TruncationMode.Fade
                        color: updateNotice.highlighted ? Theme.highlightColor : Theme.primaryColor
                        text: qsTr("Update available")
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
                model: _player ? _player.inputs : []

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
