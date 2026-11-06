// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    canAccept: players.isValidAddress(address.text)

    onAccepted: players.addManual(address.text)

    Column {
        width: parent.width

        DialogHeader {
            acceptText: qsTr("Add")
            title: qsTr("Add player")
        }

        TextField {
            id: address
            width: parent.width
            focus: true
            label: qsTr("Address, optionally with :port")
            placeholderText: "192.168.1.20"
            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText | Qt.ImhUrlCharactersOnly
            EnterKey.enabled: players.isValidAddress(text)
            EnterKey.iconSource: "image://theme/icon-m-enter-accept"
            EnterKey.onClicked: accept()
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryHighlightColor
            text: qsTr("Use this when the player is not found automatically, for example on another network segment.")
        }
    }
}
