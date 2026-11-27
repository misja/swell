// SPDX-License-Identifier: MIT
import QtQuick 2.0
import QtQml.Models 2.1   // DelegateModel
import Sailfish.Silica 1.0

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
        onCommandFailed: if (path === "/Move") reorder.putBack()
    }

    RemorsePopup { id: clearRemorse }

    DragReorder {
        id: reorder
        view: list
        delegateModel: queueModel
        proxy: dragProxy
        vertical: true
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
            dragProxy.artist = delegate.artist
            dragProxy.album = delegate.album
            dragProxy.duration = delegate.duration
            dragProxy.image = delegate.image
        }
        onMoveRequested: page._player.moveQueueEntry(from, to, page._dragPid, page._dragIdentity)
    }

    DelegateModel {
        id: queueModel
        model: page._queue

        delegate: ListItem {
            id: item

            // For the drag: what the lifted copy shows and the check needs.
            readonly property bool loaded: model.loaded
            readonly property string identity: model.identity
            readonly property string title: model.title
            readonly property string artist: model.artist
            readonly property string album: model.album
            readonly property int duration: model.duration
            readonly property url image: model.image

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

            contentHeight: Theme.itemSizeMedium
            enabled: model.loaded
            menu: page._reordering ? null : contextMenu
            highlighted: down || menuOpen || (model.current && !page._queue.streaming)
            // The lifted copy is drawn instead.
            opacity: reorder.dragged === item ? 0 : 1

            onClicked: if (!page._reordering) page._player.playQueueEntry(model.position)

            QueueRow {
                anchors.fill: parent
                loaded: model.loaded
                title: model.title
                artist: model.artist
                album: model.album
                duration: model.duration
                image: model.image
                dimmed: model.current && page._queue.streaming
                highlighted: item.highlighted
            }

            Component {
                id: contextMenu
                ContextMenu {
                    MenuItem {
                        text: qsTr("Remove")
                        onClicked: item.remove()
                    }
                }
            }

            // In the edit mode: a tap ends it; a press held 150 ms lifts the
            // row, and from then on the list, the pull-down menu and the back
            // gesture keep out (preventStealing). A press that moves earlier
            // is the list's flick.
            MouseArea {
                id: dragArea
                // This press lifted the row: its release is a drop, not a tap.
                property bool lifted
                anchors.fill: parent
                enabled: page._reordering
                preventStealing: reorder.dragged === item
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

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: page._queue !== null && page._queue.streaming && page._queue.length > 0
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("Now playing %1. Tap a track to play the queue again.")
                      .arg(page._player ? page._player.title1 : "")
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
            hintText: qsTr("Play a preset or an input, or add music from Browse")
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
    ListItem {
        id: dragProxy
        property string title
        property string artist
        property string album
        property int duration
        property url image
        visible: reorder.dragged !== null
        enabled: false
        width: list.width
        contentHeight: Theme.itemSizeMedium
        highlighted: true
        z: 1000

        QueueRow {
            anchors.fill: parent
            loaded: true
            title: dragProxy.title
            artist: dragProxy.artist
            album: dragProxy.album
            duration: dragProxy.duration
            image: dragProxy.image
            highlighted: true
        }
    }
}
