// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    allowedOrientations: Orientation.All

    SilicaListView {
        anchors.fill: parent
        header: PageHeader { title: qsTr("Players") }

        ViewPlaceholder {
            enabled: true
            text: qsTr("No players found")
        }
    }
}
