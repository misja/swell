// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

// The index on the side of a letter list, as in the People app (whose
// SymbolScrollBar is not public): spread over the full height, with a star
// above the letters when the list has favourites on top. Tap or drag to jump.
// Letters that do not fit, as in landscape, are thinned out evenly.
Item {
    id: root

    // [{ "text": "A", "row": 0 }, ...] from BrowseModel.letters
    property var letters: []
    property bool favourites
    // The letter of the part in view; highlighted.
    property string current
    signal jump(int row)
    signal top()

    readonly property int _minBox: Theme.fontSizeExtraSmall + Theme.paddingSmall
    readonly property int _slots: Math.max(1, Math.floor(height / _minBox) - (favourites ? 1 : 0))
    readonly property int _step: Math.max(1, Math.ceil(letters.length / _slots))
    readonly property var _shown: {
        var list = []
        if (favourites) {
            list.push({ "text": "", "row": -1 })
        }
        for (var i = 0; i < letters.length; i += _step) {
            list.push(letters[i])
        }
        return list
    }
    readonly property real _box: _shown.length > 0 ? height / _shown.length : height
    property int _current: -1

    width: Theme.itemSizeExtraSmall

    function _pick(y) {
        if (_shown.length === 0) {
            return
        }
        var i = Math.max(0, Math.min(_shown.length - 1, Math.floor(y / _box)))
        if (i === _current) {
            return
        }
        _current = i
        if (_shown[i].row < 0) {
            root.top()
        } else {
            root.jump(_shown[i].row)
        }
    }

    Repeater {
        model: root._shown
        Item {
            y: index * root._box
            width: root.width
            height: root._box

            Icon {
                visible: modelData.row < 0
                anchors.centerIn: parent
                width: Theme.iconSizeExtraSmall
                height: width
                source: "image://theme/icon-s-favorite"
                color: mouse.pressed && index === root._current ? Theme.highlightColor : Theme.secondaryHighlightColor
            }
            Label {
                visible: modelData.row >= 0
                anchors.centerIn: parent
                font.pixelSize: Theme.fontSizeExtraSmall
                color: (mouse.pressed && index === root._current) || (!mouse.pressed && modelData.text === root.current)
                       ? Theme.highlightColor : Theme.secondaryColor
                font.bold: !mouse.pressed && modelData.text === root.current
                text: modelData.text
            }
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        preventStealing: true
        onPressed: root._pick(mouse.y)
        onPositionChanged: root._pick(mouse.y)
        onReleased: root._current = -1
    }
}
