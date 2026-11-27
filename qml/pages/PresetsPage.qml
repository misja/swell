// SPDX-License-Identifier: MIT
import QtQuick 2.4   // FontMetrics
import QtQml.Models 2.2   // DelegateModel
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

    // The edit mode, and the drag in it. Own code (spec 008, G): Silica has
    // no reordering, and the home screen's (lipstick) is private.
    property bool _reordering: false
    readonly property bool _canReorder: _player !== null && _player.online && _player.presetsCurrent
                                        && !_player.presetMoving && _presets.length > 1
                                        && !deleteRemorse.active
    property Item _dragged: null      // the delegate whose tile is lifted
    property var _draggedPreset: null
    property int _dragStart: -1       // its index when the drag began
    property string _dragPrid
    property int _pendingIndex: -1    // where the dwell will move it
    // The last move sent, to put back if the player does not take it.
    property int _sentFrom: -1
    property int _sentTo: -1

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

    function _startDrag(delegate) {
        if (!_reordering || !_player.presetsCurrent || _player.presetMoving) {
            return
        }
        _dragged = delegate
        _draggedPreset = delegate.preset
        _dragStart = delegate.DelegateModel.itemsIndex
        _dragPrid = _player.presetsPrid
        _sentFrom = -1
        var at = delegate.mapToItem(page, 0, 0)
        dragProxy.x = at.x
        dragProxy.y = at.y
    }

    // Puts a lifted tile back where it started; nothing is sent.
    function _cancelDrag() {
        if (!_dragged) {
            return
        }
        var now = _dragged.DelegateModel.itemsIndex
        if (now !== _dragStart && _dragStart >= 0 && now >= 0) {
            presetsModel.items.move(now, _dragStart)
        }
        _endDrag()
    }

    function _endDrag() {
        dwell.stop()
        edgeScroll.stop()
        _dragged = null
        _draggedPreset = null
        _pendingIndex = -1
    }

    function _dropDrag() {
        var now = _dragged.DelegateModel.itemsIndex
        var start = _dragStart
        var preset = _draggedPreset
        var prid = _dragPrid
        _endDrag()
        if (now === start) {
            return
        }
        _sentFrom = now
        _sentTo = start
        // Slots stay where they are: the preset takes the slot that was at
        // its new place (spec 008, G).
        _player.movePreset(preset.id, _presets[now].id, prid)
    }

    function _putBack() {
        if (_sentFrom >= 0 && _sentFrom < presetsModel.items.count) {
            presetsModel.items.move(_sentFrom, _sentTo)
        }
        _sentFrom = -1
    }

    // Where the lifted tile's centre is over the grid; -1 when over none.
    function _indexUnderProxy() {
        var p = grid.mapFromItem(page, dragProxy.x + dragProxy.width / 2, dragProxy.y + dragProxy.height / 2)
        return grid.indexAt(p.x + grid.contentX, p.y + grid.contentY)
    }

    function _followFinger(item, x, y) {
        var p = item.mapToItem(page, x, y)
        dragProxy.x = p.x - dragProxy.width / 2
        dragProxy.y = p.y - dragProxy.height / 2
        _aim()
        // Near the grid's top or bottom edge, the grid scrolls.
        var g = grid.mapFromItem(page, 0, p.y).y
        var edge = Theme.itemSizeMedium
        edgeScroll.step = g < edge ? -Theme.paddingLarge
                        : g > grid.height - edge ? Theme.paddingLarge : 0
        if (edgeScroll.step !== 0) {
            edgeScroll.start()
        } else {
            edgeScroll.stop()
        }
    }

    // Held over another place for 150 ms, the tile moves there.
    function _aim() {
        var index = _indexUnderProxy()
        if (index >= 0 && _dragged && index !== _dragged.DelegateModel.itemsIndex) {
            if (index !== _pendingIndex) {
                _pendingIndex = index
                dwell.restart()
            }
        } else {
            _pendingIndex = -1
            dwell.stop()
        }
    }

    function _endReordering() {
        _cancelDrag()
        _reordering = false
    }

    onStatusChanged: if (status !== PageStatus.Active) _endReordering()
    onOrientationChanged: _cancelDrag()
    on_CanReorderChanged: if (!_player || !_player.online) _endReordering()

    Connections {
        target: page._player
        onPresetDeleteRefused: Notices.show(presetsChanged ? qsTr("The presets have changed. Nothing was deleted.")
                                                           : qsTr("Nothing was deleted."), Notice.Short)
        onPresetMoveRefused: {
            page._putBack()
            Notices.show(presetsChanged ? qsTr("The presets have changed. Nothing was moved.")
                                        : qsTr("Nothing was moved."), Notice.Short)
        }
        // The app shows the failure notice.
        onCommandFailed: if (path === "/Presets/edit") page._putBack()
        // A new list resets the view, and a drag in it ends.
        onQmlPresetsChanged: {
            page._endDrag()
            page._sentFrom = -1
        }
    }

    RemorsePopup { id: deleteRemorse }

    Timer {
        id: hold
        property Item target
        interval: 150
        onTriggered: page._startDrag(target)
    }
    Timer {
        id: dwell
        interval: 150
        onTriggered: {
            if (page._dragged && page._pendingIndex >= 0) {
                presetsModel.items.move(page._dragged.DelegateModel.itemsIndex, page._pendingIndex)
                page._pendingIndex = -1
            }
        }
    }
    Timer {
        id: edgeScroll
        property real step
        interval: 16
        repeat: true
        onTriggered: {
            var top = grid.originY
            var bottom = grid.originY + grid.contentHeight - grid.height
            var y = Math.max(top, Math.min(bottom, grid.contentY + step))
            if (y === grid.contentY) {
                stop()
            } else {
                grid.contentY = y
                page._aim()
            }
        }
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
            opacity: page._dragged === tile ? 0 : 1
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

            // In the edit mode: a tap ends it; a press held 150 ms lifts the
            // tile, and from then on the grid, the pull-down menu and the
            // back gesture keep out (preventStealing). A press that moves
            // earlier is the grid's flick.
            MouseArea {
                id: dragArea
                // This press lifted the tile: its release is a drop, not a tap.
                property bool lifted
                anchors.fill: parent
                enabled: page._reordering
                preventStealing: page._dragged === tile
                onPressed: {
                    lifted = false
                    hold.target = tile
                    hold.restart()
                }
                onPositionChanged: {
                    if (page._dragged === tile) {
                        lifted = true
                        page._followFinger(dragArea, mouse.x, mouse.y)
                    }
                }
                onReleased: {
                    hold.stop()
                    if (page._dragged === tile) {
                        lifted = true
                        page._dropDrag()
                    }
                }
                onClicked: if (!lifted) page._endReordering()
                onCanceled: {
                    hold.stop()
                    if (page._dragged === tile) {
                        page._cancelDrag()
                    }
                }
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
            hintText: qsTr("Long press a station in Browse, or an input on Home, and choose Add preset")
        }

        VerticalScrollDecorator {}
    }

    // The lifted tile, scaled as the home screen's (1.1).
    PresetTile {
        id: dragProxy
        visible: page._dragged !== null
        enabled: false
        width: grid.cellWidth
        contentHeight: grid.cellHeight
        scale: 1.1
        z: 1000
        preset: page._draggedPreset || { id: 0, name: "", image: "", inputId: "" }
        inputs: page._inputs
    }
}
