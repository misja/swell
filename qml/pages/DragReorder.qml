// SPDX-License-Identifier: MIT
import QtQuick 2.0
import QtQml.Models 2.1   // DelegateModel

// Dragging items of a view into a new order, as on the home screen: a press
// held 150 ms lifts the item, a copy (proxy) follows the finger, held 150 ms
// over another place the item moves there in the view (DelegateModel
// items.move(), the model itself unchanged), and near the top or bottom edge
// the view scrolls. Releasing emits moveRequested(from, to); the page sends
// it, and calls putBack() if the player does not take it.
// Own code (specs 008 G, 009 C): Silica has no reordering, and the home
// screen's (lipstick) is private. Used by the Presets and Queue pages.
Item {
    id: reorder

    // The view (GridView or ListView), its DelegateModel, and the lifted
    // copy, drawn above the view in coordinates of proxy.parent.
    property Flickable view
    property DelegateModel delegateModel
    property Item proxy
    // Whether a press may lift this delegate now (the page's conditions).
    property var canLift: function(delegate) { return true }
    // A list: the copy moves only up and down, at the item's own x.
    property bool vertical
    // The edge band and the step per 16 ms of edge scrolling.
    property real edge: 80
    property real step: 20

    readonly property Item dragged: _dragged
    readonly property int startIndex: _start

    signal lifted(Item delegate)
    signal moveRequested(int from, int to)

    property Item _dragged: null
    property int _start: -1
    property int _pending: -1
    property int _sentFrom: -1
    property int _sentTo: -1
    property Item _pressed: null

    // A press on delegate; it lifts after the hold unless released or moved
    // into a flick first.
    function press(delegate) {
        _pressed = delegate
        hold.restart()
    }

    // The finger moved over item at (x, y) while dragging.
    function follow(item, x, y) {
        if (!_dragged) {
            return
        }
        var p = item.mapToItem(proxy.parent, x, y)
        if (!vertical) {
            proxy.x = p.x - proxy.width / 2
        }
        proxy.y = p.y - proxy.height / 2
        _aim()
        var v = view.mapFromItem(proxy.parent, 0, p.y).y
        edgeScroll.direction = v < edge ? -1 : v > view.height - edge ? 1 : 0
        if (edgeScroll.direction !== 0) {
            edgeScroll.start()
        } else {
            edgeScroll.stop()
        }
    }

    // Released: a drop if lifted. Returns whether the press had lifted.
    function release() {
        hold.stop()
        if (!_dragged) {
            return false
        }
        var now = _dragged.DelegateModel.itemsIndex
        var from = _start
        end()
        if (now !== from && from >= 0 && now >= 0) {
            _sentFrom = now
            _sentTo = from
            moveRequested(from, now)
        }
        return true
    }

    // Puts a lifted item back where it started; nothing is sent.
    function cancel() {
        hold.stop()
        if (!_dragged) {
            return
        }
        var now = _dragged.DelegateModel.itemsIndex
        if (now !== _start && _start >= 0 && now >= 0) {
            delegateModel.items.move(now, _start)
        }
        end()
    }

    // Ends a drag without moving anything back (the view was reset).
    function end() {
        hold.stop()
        dwell.stop()
        edgeScroll.stop()
        if (_dragged) {
            _dragged.DelegateModel.inPersistedItems = false
        }
        _dragged = null
        _pressed = null
        _start = -1
        _pending = -1
    }

    // The last move sent was not taken: put the view back.
    function putBack() {
        if (_sentFrom >= 0 && _sentFrom < delegateModel.items.count && _sentTo < delegateModel.items.count) {
            delegateModel.items.move(_sentFrom, _sentTo)
        }
        forget()
    }

    // The last move sent no longer needs putting back (taken, or a new view).
    function forget() {
        _sentFrom = -1
        _sentTo = -1
    }

    function _lift(delegate) {
        // The delegate may be gone (a new list) by the time the hold ends.
        if (!delegate || !canLift(delegate)) {
            return
        }
        // Kept while lifted, also when edge scrolling takes it out of view.
        delegate.DelegateModel.inPersistedItems = true
        _dragged = delegate
        _start = delegate.DelegateModel.itemsIndex
        forget()
        var at = delegate.mapToItem(proxy.parent, 0, 0)
        proxy.x = at.x
        proxy.y = at.y
        lifted(delegate)
    }

    // Held over another place for 150 ms, the item moves there.
    function _aim() {
        var c = view.mapFromItem(proxy.parent, proxy.x + proxy.width / 2, proxy.y + proxy.height / 2)
        var index = view.indexAt(c.x + view.contentX, c.y + view.contentY)
        if (index >= 0 && _dragged && index !== _dragged.DelegateModel.itemsIndex) {
            if (index !== _pending) {
                _pending = index
                dwell.restart()
            }
        } else {
            _pending = -1
            dwell.stop()
        }
    }

    Timer {
        id: hold
        interval: 150
        onTriggered: reorder._lift(reorder._pressed)
    }
    Timer {
        id: dwell
        interval: 150
        onTriggered: {
            if (reorder._dragged && reorder._pending >= 0) {
                reorder.delegateModel.items.move(reorder._dragged.DelegateModel.itemsIndex, reorder._pending)
                reorder._pending = -1
            }
        }
    }
    Timer {
        id: edgeScroll
        property int direction
        interval: 16
        repeat: true
        onTriggered: {
            var v = reorder.view
            var top = v.originY
            var bottom = v.originY + v.contentHeight - v.height
            var y = Math.max(top, Math.min(bottom, v.contentY + direction * reorder.step))
            if (y === v.contentY) {
                stop()
            } else {
                v.contentY = y
                reorder._aim()
            }
        }
    }
}
