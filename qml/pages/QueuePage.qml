// SPDX-License-Identifier: MIT
import QtQuick 2.0
import QtQml.Models 2.1   // DelegateModel
import Sailfish.Silica 1.0
import Sailfish.Media 1.0   // MediaListItem

Page {
    id: page
    objectName: "queue"

    readonly property QtObject _player: players.current
    readonly property QtObject _queue: _player ? _player.queue : null
    property QtObject _previousQueue: null

    // The edit mode (spec 009, C); the drag is DragReorder's.
    property bool _reordering: false
    // The last remove remorse started. All run the same time, so it ends
    // last, unless it is undone while an earlier one still runs: then Reorder
    // is enabled early. A drag does not start while the remove is sent
    // (queueEditing), and a move checks the entry with the player first.
    property Item _remorse: null
    readonly property bool _canReorder: _player !== null && _player.online && _queue !== null
                                        && _queue.length > 1 && !_player.queueEditing
                                        && !(_remorse && _remorse.pending)
    // What the drag was started on, for the player's check before /Move.
    property string _dragPid
    property string _dragIdentity

    allowedOrientations: Orientation.All

    // The Binding leaves the old player's queue active: switch it off.
    on_QueueChanged: {
        if (_previousQueue) {
            _previousQueue.active = false
        }
        _previousQueue = _queue
    }
    Component.onCompleted: _previousQueue = _queue

    function _positionAtCurrent() {
        if (_queue && _queue.currentIndex >= 0 && _queue.currentIndex < list.count) {
            list.positionViewAtIndex(_queue.currentIndex, ListView.Center)
        }
    }

    // "artist · album", either alone, as before (spec 002).
    function _subtitle(artist, album) {
        if (artist !== "" && album !== "") {
            return artist + " · " + album
        }
        return artist + album
    }

    function _endReordering() {
        reorder.cancel()
        _reordering = false
    }

    onStatusChanged: if (status !== PageStatus.Active) _endReordering()
    onOrientationChanged: reorder.cancel()
    on_CanReorderChanged: {
        if (!_player || !_player.online || !_queue || _queue.length < 2) {
            _endReordering()
        }
    }

    // The queue loads only while this page is on screen (spec 002).
    Binding {
        target: page._queue
        property: "active"
        // Activating too: start loading while the page is swiped in.
        value: (page.status === PageStatus.Activating || page.status === PageStatus.Active)
               && Qt.application.active
        when: page._queue !== null
    }

    Connections {
        target: page._queue
        // A new queue: a drag in it ends, the mode stays (as for presets).
        onModelReset: {
            reorder.end()
            reorder.forget()
        }
        onReloaded: {
            reorder.end()
            reorder.forget()
            page._positionAtCurrent()
        }
    }

    Connections {
        target: page._player
        onRemoveRefused: Notices.show(qsTr("The queue has changed. Nothing was removed."), Notice.Short)
        onMoveRefused: {
            reorder.putBack()
            Notices.show(queueChanged ? qsTr("The queue has changed. Nothing was moved.")
                                      : qsTr("Nothing was moved."), Notice.Short)
        }
        // The app shows the failure notice.
        onMoveFailed: reorder.putBack()
    }

    RemorsePopup { id: clearRemorse }

    DragReorder {
        id: reorder
        view: list
        delegateModel: queueModel
        proxy: dragProxy
        edge: Theme.itemSizeMedium
        step: Theme.paddingLarge
        // Only loaded rows: a placeholder has no identity to check.
        canLift: function(delegate) {
            return page._reordering && delegate.loaded && !page._player.queueEditing
        }
        onLifted: {
            page._dragPid = page._queue.pid
            page._dragIdentity = delegate.identity
            dragProxy.title = delegate.title
            dragProxy.subtitle = delegate.subtitle
            dragProxy.duration = delegate.duration
        }
        onMoveRequested: page._player.moveQueueEntry(from, to, page._dragPid, page._dragIdentity)
    }

    DelegateModel {
        id: queueModel
        model: page._queue

        // The Media app's row (Sailfish.Media MediaListItem): duration, title,
        // "artist · album", the playing entry highlighted.
        delegate: MediaListItem {
            id: item

            // For the drag: whether it can lift, and the player's check.
            readonly property bool loaded: model.loaded
            readonly property string identity: model.identity

            function remove() {
                // The player confirms pid and identity before deleting. The row's
                // index follows earlier removals; once the row is gone, the
                // position recorded here is the best guess left.
                var pid = page._queue.pid
                var identity = model.identity
                var position = model.position
                page._remorse = remorseAction(qsTr("Removing"), function() {
                    var now = typeof index === "number" && index >= 0 ? index : position
                    page._player.removeQueueEntry(now, pid, identity)
                })
            }

            enabled: model.loaded
            menu: page._reordering ? null : contextMenu
            highlighted: down || menuOpen
            // While an input or stream plays, the current entry is only where
            // playback resumes (spec 002): not marked as playing.
            playing: model.current && !page._queue.streaming
            duration: model.loaded && model.duration > 0 ? model.duration : undefined
            title: model.loaded ? model.title : "…"
            // Titles from the player are text, not markup.
            textFormat: Text.PlainText
            subtitle: model.loaded ? page._subtitle(model.artist, model.album) : ""
            // The lifted copy is drawn instead.
            opacity: reorder.dragged === item ? 0 : 1

            onClicked: if (!page._reordering) page._player.playQueueEntry(model.position)

            Component {
                id: contextMenu
                ContextMenu {
                    MenuItem {
                        text: qsTr("Remove")
                        onClicked: item.remove()
                    }
                }
            }

            // In the edit mode: a tap ends it, and a press held 150 ms lifts
            // the row, which Qt then moves up and down (drag). The list, the
            // pull-down menu and the back gesture keep out for the whole mode
            // (preventStealing); the list scrolls at its edges only.
            MouseArea {
                id: dragArea
                // This press lifted the row: its release is a drop, not a tap.
                property bool lifted
                anchors.fill: parent
                enabled: page._reordering
                preventStealing: page._reordering
                drag.target: reorder.dragged === item ? dragProxy : null
                drag.axis: Drag.YAxis
                drag.minimumY: 0
                drag.maximumY: page.height - dragProxy.height
                onPressed: {
                    lifted = false
                    reorder.press(item)
                }
                onPositionChanged: {
                    if (reorder.dragged === item) {
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

    SilicaListView {
        id: list
        anchors.fill: parent
        model: queueModel

        // The others make room as the home screen's do.
        move: Transition {
            NumberAnimation { properties: "y"; duration: 150; easing.type: Easing.InOutQuad }
        }
        moveDisplaced: Transition {
            NumberAnimation { properties: "y"; duration: 150; easing.type: Easing.InOutQuad }
        }

        header: Column {
            width: list.width

            PageHeader {
                title: qsTr("Queue")
                description: {
                    if (page._reordering) {
                        return qsTr("Drag to move, tap to stop")
                    }
                    if (!page._player) {
                        return ""
                    }
                    var name = page._player.name !== "" ? page._player.name : page._player.host
                    if (!page._queue || !page._queue.known || page._queue.length === 0) {
                        return name
                    }
                    return qsTr("%n tracks", "", page._queue.length) + " · " + name
                }
            }

            Item {
                width: 1
                height: Theme.paddingMedium
            }
        }

        PullDownMenu {
            visible: page._queue !== null && page._queue.length > 0

            MenuItem {
                text: qsTr("Reorder")
                enabled: !page._reordering && page._canReorder
                onClicked: page._reordering = true
            }
            MenuItem {
                text: qsTr("Clear queue")
                enabled: !page._reordering
                onClicked: clearRemorse.execute(qsTr("Clearing queue"), function() {
                    page._player.clearQueue()
                })
            }
        }

        // In the edit mode, a tap beside the rows ends it; the rows end it
        // themselves.
        MouseArea {
            y: list.originY
            z: -1
            width: list.width
            height: Math.max(list.contentHeight, list.height)
            enabled: page._reordering
            onClicked: page._endReordering()
        }

        ViewPlaceholder {
            enabled: page._queue !== null && page._queue.known && page._queue.length === 0
            text: qsTr("Queue is empty")
            hintText: qsTr("Play a preset or an input, or add music")
        }

        BusyIndicator {
            anchors.centerIn: parent
            size: BusySize.Large
            running: page._queue !== null && !page._queue.known && page._queue.loading
        }

        VerticalScrollDecorator {}
    }

    // The lifted row, with the pressed row's highlight so it reads as one row
    // over the others. Not scaled: a full-width row would leave the screen.
    MediaListItem {
        id: dragProxy
        visible: reorder.dragged !== null
        enabled: false
        width: list.width
        highlighted: true
        z: 1000
    }
}
