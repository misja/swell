// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

// A cover with the placeholder of Jolla's Media player (AlbumArt.qml in
// com.jolla.mediaplayer): a faint gradient with the albums icon while the
// image loads, or when there is none. The image fades in once it is ready.
Item {
    id: root

    property alias source: image.source
    property bool highlighted
    // Off for images that are icons rather than covers (TuneIn's menus).
    property bool placeholder: true
    property string icon: "image://theme/icon-m-media-albums"

    Rectangle {
        anchors.fill: parent
        // Until the cover has faded in completely, so no gap shows between.
        visible: root.placeholder && (image.status !== Image.Ready || image.opacity < 1.0)
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.rgba(Theme.primaryColor, Theme.opacityFaint) }
            GradientStop { position: 1.0; color: Theme.rgba(Theme.primaryColor, 0.05) }
        }

        Icon {
            anchors.centerIn: parent
            // The medium icon, smaller where the cover itself is small.
            width: Math.min(Theme.iconSizeMedium, Math.round(root.width * 0.6))
            height: width
            sourceSize.width: width
            sourceSize.height: height
            source: root.icon
            highlighted: root.highlighted
        }
    }

    Image {
        id: image
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        sourceSize.width: width
        sourceSize.height: height
        opacity: status === Image.Ready ? 1.0 : 0.0
        Behavior on opacity { FadeAnimation {} }
    }
}
