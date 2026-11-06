// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

// A column of letters on the side of a letter list; tap or drag to jump, as
// in the People app (whose SymbolScrollBar is not public). Letters that do
// not fit, as in landscape, are thinned out evenly.
Item {
    id: root

    // [{ "text": "A", "row": 0 }, ...] from BrowseModel.letters
    property var letters: []
    signal jump(int row)

    readonly property int _boxSize: Theme.iconSizeSmall
    readonly property int _fits: Math.max(1, Math.floor(height / _boxSize))
    readonly property int _step: Math.max(1, Math.ceil(letters.length / _fits))
    readonly property var _shown: {
        var list = []
        for (var i = 0; i < letters.length; i += _step) {
            list.push(letters[i])
        }
        return list
    }
    property string _current

    width: Theme.itemSizeExtraSmall

    function _pick(y) {
        if (_shown.length === 0) {
            return
        }
        var i = Math.max(0, Math.min(_shown.length - 1, Math.floor(y / (column.height / _shown.length))))
        if (_shown[i].text !== _current) {
            _current = _shown[i].text
            root.jump(_shown[i].row)
        }
    }

    Column {
        id: column
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width

        Repeater {
            model: root._shown
            Label {
                width: column.width
                height: root._boxSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: Theme.fontSizeExtraSmall
                color: mouse.pressed && modelData.text === root._current ? Theme.highlightColor : Theme.secondaryColor
                text: modelData.text
            }
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: column
        preventStealing: true
        onPressed: root._pick(mouse.y)
        onPositionChanged: root._pick(mouse.y)
        onReleased: root._current = ""
    }
}
