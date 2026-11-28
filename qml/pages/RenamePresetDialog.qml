// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

// Renames a preset (spec 008, I), as Jolla's file manager renames a file
// (Sailfish.FileManager RenameDialog.qml): a header, the name pre-filled and
// selected, accept with the enter key.
Dialog {
    id: dialog

    property string name
    readonly property string newName: field.text.trim()

    allowedOrientations: Orientation.All
    canAccept: newName !== "" && newName !== name

    DialogHeader {
        id: header
        title: qsTr("Rename")
    }

    TextField {
        id: field
        anchors.top: header.bottom
        width: parent.width
        label: qsTr("Name")
        placeholderText: qsTr("Name")
        text: dialog.name
        // As Jolla's RenameDialog: focus once created, then select the name.
        Component.onCompleted: focus = true
        onFocusChanged: if (focus) selectAll()

        EnterKey.iconSource: "image://theme/icon-m-enter-accept"
        EnterKey.enabled: dialog.canAccept
        EnterKey.onClicked: dialog.accept()
    }
}
