// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    objectName: "about"
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
                text: qsTr("Swell %1").arg(appVersion)
            }

            Label {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("A Sailfish OS controller for BluOS players.")
            }

            SectionHeader { text: qsTr("Privacy") }

            Label {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("Swell connects only to your players and to the servers their artwork comes from. There is no account, and no data is collected or shared. Only the addresses you add and your last player are stored, on this device.")
            }

            SectionHeader { text: qsTr("License") }

            Label {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("MIT License. Source: %1").arg("https://github.com/misja/swell")
            }

            Label {
                width: parent.width
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("Not affiliated with or endorsed by Lenbrook Industries. BluOS is a trademark of Lenbrook Industries Limited.")
            }
        }

        VerticalScrollDecorator {}
    }
}
