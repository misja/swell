// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.All

    // Every player polls only while this list is on screen (spec: background polling).
    Binding {
        target: players
        property: "pollAll"
        value: page.status === PageStatus.Active && Qt.application.active
    }

    function availabilityText(availability) {
        switch (availability) {
        case 0: return qsTr("Connecting…")
        case 2: return qsTr("Not reachable")
        case 3: return qsTr("Not a BluOS player")
        }
        return ""
    }

    SilicaListView {
        id: list
        anchors.fill: parent
        model: players

        header: PageHeader { title: qsTr("Players") }

        PullDownMenu {
            busy: players.searching
            MenuItem {
                text: qsTr("About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }
            MenuItem {
                text: qsTr("Add player…")
                onClicked: pageStack.push(Qt.resolvedUrl("AddPlayerDialog.qml"))
            }
            MenuItem {
                text: qsTr("Search again")
                onClicked: players.search()
            }
        }

        delegate: ListItem {
            id: item
            contentHeight: Theme.itemSizeMedium
            menu: model.manual ? removeMenu : null
            highlighted: down || model.playerId === players.currentId

            onClicked: {
                players.currentId = model.playerId
                pageStack.push(Qt.resolvedUrl("HomePage.qml"))
            }

            Column {
                anchors {
                    left: parent.left
                    right: parent.right
                    leftMargin: Theme.horizontalPageMargin
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                Label {
                    width: parent.width
                    text: model.name
                    truncationMode: TruncationMode.Fade
                    color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                Label {
                    width: parent.width
                    font.pixelSize: Theme.fontSizeExtraSmall
                    truncationMode: TruncationMode.Fade
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    text: model.availability === 1
                          ? (model.nowPlaying !== "" ? model.nowPlaying : model.modelName)
                          : page.availabilityText(model.availability)
                }
            }

            Component {
                id: removeMenu
                ContextMenu {
                    MenuItem {
                        text: qsTr("Remove")
                        onClicked: players.removeManual(model.playerId)
                    }
                }
            }
        }

        ViewPlaceholder {
            enabled: list.count === 0 && !players.searching
            text: qsTr("No players found")
            hintText: qsTr("Pull down to search again or to add a player by address")
        }

        BusyIndicator {
            anchors.centerIn: parent
            size: BusySize.Large
            running: list.count === 0 && players.searching
        }

        VerticalScrollDecorator {}
    }
}
