// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

// One preset in a grid of presets (Home's row, the Presets page): a Silica
// GridItem that fills its cell (GridItem.qml:47-48), with the cover, or for
// an input its icon, and the name below. Tapping plays it; the page that
// shows it sets the menu, if any.
GridItem {
    id: tile

    // { id, name, image, inputId } from Player.presets
    property var preset
    // Player.inputs: a preset that selects an input shows the icon the user
    // chose for that input in the official app; the preset itself carries
    // only a generic one.
    property var inputs: []

    readonly property bool _isInput: preset.inputId !== ""
    readonly property real _art: width

    function _inputImage() {
        for (var i = 0; i < inputs.length; ++i) {
            if (inputs[i].id === preset.inputId) {
                return inputs[i].image
            }
        }
        return ""
    }

    // Inputs are line icons: give them a tile like the station logos.
    Rectangle {
        width: tile._art
        height: tile._art
        visible: tile._isInput
        color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
    }
    Image {
        id: inputArt
        visible: tile._isInput
        x: (tile._art - width) / 2
        y: (tile._art - height) / 2
        width: tile._art * 0.55
        height: width
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        sourceSize.width: width
        sourceSize.height: height
        source: tile._isInput ? tile._inputImage() || tile.preset.image : ""
    }
    Icon {
        x: (tile._art - width) / 2
        y: (tile._art - height) / 2
        visible: tile._isInput && inputArt.status !== Image.Ready
        source: "image://theme/icon-m-media-radio"
    }
    // Stations, albums and playlists: the cover placeholder.
    CoverImage {
        visible: !tile._isInput
        width: tile._art
        height: width
        highlighted: tile.highlighted
        icon: "image://theme/icon-m-media-radio"
        source: tile._isInput ? "" : tile.preset.image
    }
    Label {
        x: Theme.paddingSmall
        y: tile._art + Theme.paddingSmall
        width: parent.width - 2 * Theme.paddingSmall
        font.pixelSize: Theme.fontSizeExtraSmall
        truncationMode: TruncationMode.Fade
        color: tile.highlighted ? Theme.highlightColor : Theme.primaryColor
        text: tile.preset.name
    }
}
