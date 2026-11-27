// SPDX-License-Identifier: MIT
import QtQuick 2.4   // FontMetrics
import QtQml.Models 2.1   // DelegateModel
import Sailfish.Silica 1.0
import Swell 1.0

// All presets of the current player (spec 008): a standard Silica grid of
// GridItems, each with a menu to delete it behind a remorse. "Reorder" in
// the pull-down menu starts an edit mode as on the home screen: a tile held
// 150 ms is lifted and dragged, each drop moves one preset, and a tap ends
// the mode.
Page {
    id: page
    objectName: "presets"

    readonly property QtObject _player: players.current
    readonly property var _inputs: _player ? _player.inputs : []
    // As in Jolla's Gallery (Sailfish.Gallery ImageGridView.qml).
    readonly property int _columns: Math.max(1, Math.floor(width / Theme.itemSizeHuge))
    // In slot order: a move is between slots, so the view's order is theirs.
    readonly property var _presets: {
        var list = _player ? _player.presets.slice(0) : []
        list.sort(function(a, b) { return a.id - b.id })
        return list
    }

    // The edit mode (spec 008, G); the drag is DragReorder's.
    property bool _reordering: false
    readonly property bool _canReorder: _player !== null && _player.online && _player.presetsCurrent
                                        && !_player.presetMoving && _presets.length > 1
                                        && !deleteRemorse.active
    property var _draggedPreset: null
    property string _dragPrid

    allowedOrientations: Orientation.All

    // The remorse is the page's, not the tile's: a presets change recreates
    // the tiles. It keeps the player and the version of the list the user
    // saw. Leaving the page carries it out at once (RemorsePopup.qml:186).
    function _deletePreset(id, name) {
        var player = _player
        var prid = player.presetsPrid
        deleteRemorse.execute(qsTr("Deleting %1").arg(name), function() {
            if (!player || players.current !== player) {
                Notices.show(qsTr("Nothing was deleted."), Notice.Short)
                return
            }
            player.deletePreset(id, prid)
        })
    }

    function _endReordering() {
        reorder.cancel()
        _reordering = false
    }

    onStatusChanged: if (status !== PageStatus.Active) _endReordering()
    onOrientationChanged: reorder.cancel()
    on_CanReorderChanged: if (!_player || !_player.online) _endReordering()

    Connections {
        target: page._player
        onPresetDeleteRefused: Notices.show(presetsChanged ? qsTr("The presets have changed. Nothing was deleted.")
                                                           : qsTr("Nothing was deleted."), Notice.Short)
        onPresetMoveRefused: {
            reorder.putBack()
            Notices.show(presetsChanged ? qsTr("The presets have changed. Nothing was moved.")
                                        : qsTr("Nothing was moved."), Notice.Short)
        }
        // The app shows the failure notice.
        onCommandFailed: if (path === "/Presets/edit") reorder.putBack()
        // A new list resets the view, and a drag in it ends.
        onQmlPresetsChanged: {
            reorder.end()
            reorder.forget()
        }
    }

    RemorsePopup { id: deleteRemorse }

    DragReorder {
        id: reorder
        view: grid
        delegateModel: presetsModel
        proxy: dragProxy
        edge: Theme.itemSizeMedium
        step: Theme.paddingLarge
        canLift: function(delegate) {
            return page._reordering && page._player.presetsCurrent && !page._player.presetMoving
        }
        onLifted: {
            page._draggedPreset = delegate.preset
            page._dragPrid = page._player.presetsPrid
        }
        // Slots stay where they are: the preset takes the slot that was at
        // its new place (spec 008, G).
        onMoveRequested: page._player.movePreset(page._draggedPreset.id, page._presets[to].id, page._dragPrid)
    }

    DelegateModel {
        id: presetsModel
        model: page._presets
        delegate: PresetTile {
            id: tile
            preset: modelData
            inputs: page._inputs
            enabled: page._player.online
            // The lifted copy is drawn instead.
            opacity: reorder.dragged === tile ? 0 : 1
            onClicked: page._player.playPreset(modelData.id)
            menu: page._reordering ? null : deleteMenu

            Component {
                id: deleteMenu
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

            // In the edit mode: a tap ends it, and a press held 150 ms lifts
            // the tile, which Qt then moves (drag). The grid, the pull-down
            // menu and the back gesture keep out for the whole mode
            // (preventStealing); the grid scrolls at its edges only.
            MouseArea {
                id: dragArea
                // This press lifted the tile: its release is a drop, not a tap.
                property bool lifted
                anchors.fill: parent
                enabled: page._reordering
                preventStealing: page._reordering
                drag.target: reorder.dragged === tile ? dragProxy : null
                drag.axis: Drag.XAndYAxis
                drag.minimumX: 0
                drag.maximumX: page.width - dragProxy.width
                drag.minimumY: 0
                drag.maximumY: page.height - dragProxy.height
                onPressed: {
                    lifted = false
                    reorder.press(tile)
                }
                onPositionChanged: {
                    if (reorder.dragged === tile) {
                        lifted = true
                        reorder.follow(dragArea, mouse.x, mouse.y)
                    }
                }
                onReleased: if (reorder.release()) lifted = true
                onClicked: if (!lifted) page._endReordering()
                onCanceled: reorder.cancel()
            }
        }
    }

    SilicaGridView {
        id: grid
        anchors.fill: parent
        cellWidth: Math.floor(width / page._columns)
        cellHeight: cellWidth + nameFont.height + 2 * Theme.paddingSmall
        model: presetsModel

        // The others slide aside as the home screen's do.
        move: Transition {
            NumberAnimation { properties: "x,y"; duration: 150; easing.type: Easing.InOutQuad }
        }
        displaced: Transition {
            NumberAnimation { properties: "x,y"; duration: 150; easing.type: Easing.InOutQuad }
        }

        FontMetrics {
            id: nameFont
            font.pixelSize: Theme.fontSizeExtraSmall
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Reorder")
                enabled: !page._reordering && page._canReorder
                onClicked: page._reordering = true
            }
        }

        // In the edit mode, a tap beside the tiles ends it, as on the home
        // screen; the tiles end it themselves.
        MouseArea {
            y: grid.originY
            z: -1
            width: grid.width
            height: Math.max(grid.contentHeight, grid.height)
            enabled: page._reordering
            onClicked: page._endReordering()
        }

        header: PageHeader {
            title: qsTr("Presets")
            description: page._reordering ? qsTr("Drag to move, tap to stop")
                                          : page._player ? (page._player.name !== "" ? page._player.name : page._player.host) : ""
        }

        ViewPlaceholder {
            enabled: grid.count === 0 && page._player !== null && page._player.online
            text: qsTr("No presets")
            hintText: qsTr("Long press a station in Music, or an input on Home, and choose Add preset")
        }

        VerticalScrollDecorator {}
    }

    // The lifted tile, scaled as the home screen's (1.1).
    PresetTile {
        id: dragProxy
        visible: reorder.dragged !== null
        enabled: false
        width: grid.cellWidth
        contentHeight: grid.cellHeight
        scale: 1.1
        z: 1000
        preset: page._draggedPreset || { id: 0, name: "", image: "", inputId: "" }
        inputs: page._inputs
    }
}
