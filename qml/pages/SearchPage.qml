// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

// Search text for a source, or for everything (spec 004). Sent on Enter; the
// results open as a Browse page.
Page {
    id: page

    // A source's searchKey; empty searches everywhere.
    property string searchKey
    property string sourceName

    allowedOrientations: Orientation.All

    function _search() {
        var text = field.text.trim()
        if (text === "") {
            return
        }
        pageStack.push(Qt.resolvedUrl("BrowsePage.qml"), {
            key: page.searchKey,
            query: text,
            title: text,
            inheritedSearchKey: page.searchKey
        })
    }

    Column {
        width: parent.width

        PageHeader {
            title: qsTr("Search")
            description: page.sourceName !== "" ? page.sourceName : qsTr("Everywhere")
        }

        SearchField {
            id: field
            width: parent.width
            placeholderText: qsTr("Artist, album, track or station")
            inputMethodHints: Qt.ImhNoPredictiveText
            focus: true
            EnterKey.iconSource: "image://theme/icon-m-search"
            EnterKey.enabled: text.trim() !== ""
            EnterKey.onClicked: page._search()
        }
    }
}
