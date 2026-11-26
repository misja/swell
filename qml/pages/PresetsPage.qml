// SPDX-License-Identifier: MIT
import QtQuick 2.4   // FontMetrics
import Sailfish.Silica 1.0
import Swell 1.0

// All presets of the current player (spec 008): a standard Silica grid of
// GridItems, each with a menu to delete it behind a remorse. Presets are
// added from Browse.
Page {
    id: page
    objectName: "presets"

    readonly property QtObject _player: players.current
    readonly property var _inputs: _player ? _player.inputs : []
    // As in Jolla's Gallery (Sailfish.Gallery ImageGridView.qml).
    readonly property int _columns: Math.max(1, Math.floor(width / Theme.itemSizeHuge))

    allowedOrientations: Orientation.All

    // The remorse is the page's, not the tile's: a presets change recreates
    // the tiles. It keeps the player and the version of the list the user
    // saw. Leaving the page carries it out at once (RemorsePopup.qml:186).
    function _deletePreset(id, name) {
        var player = _player
        var prid = player.presetsPrid
        deleteRemorse.execute(qsTr("Deleting %1").arg(name), function() {
            if (!player || players.current !== player) {
                app.notify(qsTr("Nothing was deleted."))
                return
            }
            player.deletePreset(id, prid)
        })
    }

    Connections {
        target: page._player
        onPresetDeleteRefused: app.notify(presetsChanged ? qsTr("The presets have changed. Nothing was deleted.")
                                                         : qsTr("Nothing was deleted."))
    }

    RemorsePopup { id: deleteRemorse }

    SilicaGridView {
        id: grid
        anchors.fill: parent
        cellWidth: Math.floor(width / page._columns)
        cellHeight: cellWidth + nameFont.height + 2 * Theme.paddingSmall
        model: page._player ? page._player.presets : []

        FontMetrics {
            id: nameFont
            font.pixelSize: Theme.fontSizeExtraSmall
        }

        header: PageHeader {
            title: qsTr("Presets")
            description: page._player ? (page._player.name !== "" ? page._player.name : page._player.host) : ""
        }

        delegate: PresetTile {
            preset: modelData
            inputs: page._inputs
            enabled: page._player.online
            onClicked: page._player.playPreset(modelData.id)
            menu: Component {
                ContextMenu {
                    MenuItem {
                        text: qsTr("Delete")
                        // One at a time, and only on the list as the player
                        // has it now.
                        enabled: !deleteRemorse.active && page._player.presetsCurrent
                        onClicked: page._deletePreset(modelData.id, modelData.name)
                    }
                }
            }
        }

        ViewPlaceholder {
            enabled: grid.count === 0 && page._player !== null && page._player.online
            text: qsTr("No presets")
            hintText: qsTr("Add them from Browse: long press a station and choose Add preset")
        }

        VerticalScrollDecorator {}
    }
}
