// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    objectName: "queue"

    readonly property QtObject _player: players.current
    readonly property QtObject _queue: _player ? _player.queue : null
    property QtObject _previousQueue: null

    allowedOrientations: Orientation.All

    function _formatTime(secs) {
        if (secs < 0) {
            return ""
        }
        var m = Math.floor(secs / 60)
        var s = secs % 60
        return m + ":" + (s < 10 ? "0" : "") + s
    }

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

    // The queue loads only while this page is on screen (spec 002).
    Binding {
        target: page._queue
        property: "active"
        value: page.status === PageStatus.Active && Qt.application.active
        when: page._queue !== null
    }

    Connections {
        target: page._queue
        onReloaded: page._positionAtCurrent()
    }

    Connections {
        target: page._player
        onCommandFailed: {
            console.warn("Command", path, "failed:", error)
            app.notify(qsTr("The player did not accept that."))
        }
        onRemoveRefused: app.notify(qsTr("The queue has changed. Nothing was removed."))
    }

    RemorsePopup { id: clearRemorse }

    SilicaListView {
        id: list
        anchors.fill: parent
        model: page._queue

        header: Column {
            width: list.width

            PageHeader {
                title: qsTr("Queue")
                description: {
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
                text: qsTr("Clear queue")
                onClicked: clearRemorse.execute(qsTr("Clearing queue"), function() {
                    page._player.clearQueue()
                })
            }
        }

        delegate: ListItem {
            id: item

            function remove() {
                // The player confirms pid and identity before deleting. The row's
                // index follows earlier removals; once the row is gone, the
                // position recorded here is the best guess left.
                var pid = page._queue.pid
                var identity = model.identity
                var position = model.position
                remorseAction(qsTr("Removing"), function() {
                    var now = typeof index === "number" && index >= 0 ? index : position
                    page._player.removeQueueEntry(now, pid, identity)
                })
            }

            contentHeight: Theme.itemSizeMedium
            enabled: model.loaded
            menu: contextMenu
            highlighted: down || menuOpen || (model.current && !page._queue.streaming)

            onClicked: page._player.playQueueEntry(model.position)

            Image {
                id: art
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.itemSizeMedium - 2 * Theme.paddingSmall
                height: width
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                sourceSize.width: width
                sourceSize.height: height
                source: model.image
            }

            Column {
                anchors {
                    left: art.right
                    leftMargin: Theme.paddingLarge
                    right: duration.left
                    rightMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                    opacity: model.current && page._queue.streaming ? Theme.opacityHigh : 1.0
                    text: model.loaded ? model.title : "…"
                }
                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    text: {
                        if (!model.loaded) {
                            return ""
                        }
                        if (model.artist !== "" && model.album !== "") {
                            return model.artist + " · " + model.album
                        }
                        return model.artist + model.album
                    }
                }
            }

            Label {
                id: duration
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                font.pixelSize: Theme.fontSizeExtraSmall
                color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                text: model.loaded ? page._formatTime(model.duration) : ""
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
        }

        ViewPlaceholder {
            enabled: page._queue !== null && page._queue.known && page._queue.length === 0
            text: qsTr("Queue is empty")
            hintText: qsTr("Play a preset or an input from Home")
        }

        BusyIndicator {
            anchors.centerIn: parent
            size: BusySize.Large
            running: page._queue !== null && !page._queue.known && page._queue.loading
        }

        VerticalScrollDecorator {}
    }
}
