// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            spacing: Theme.paddingLarge

            PageHeader { title: qsTr("About Swell") }

            Label {
                width: parent.width
                wrapMode: Text.Wrap
                text: qsTr("Swell %1 (works with BluOS)").arg(appVersion)
            }

            Label {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("An independent controller for BluOS players on your local network. It is not made, endorsed or supported by Lenbrook.")
            }

            SectionHeader { text: qsTr("Privacy") }

            Label {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("Swell talks only to players on your local network and to the image servers their artwork comes from. It has no account and collects, sends and shares no personal data. It stores only the player addresses you add and the player you last chose, on this device.")
            }

            SectionHeader { text: qsTr("License") }

            Label {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("MIT License. Source: %1").arg("https://github.com/misja/swell")
            }
        }

        VerticalScrollDecorator {}
    }
}
