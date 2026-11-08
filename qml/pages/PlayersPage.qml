// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    objectName: "players"

    allowedOrientations: Orientation.All

    // Shows the current player's Home, one swipe to the left of this list.
    function openCurrent(immediate) {
        if (!players.current) {
            return
        }
        if (pageStack.nextPage(page) === null) {
            pageStack.pushAttached(Qt.resolvedUrl("HomePage.qml"))
        }
        pageStack.navigateForward(immediate ? PageStackAction.Immediate : PageStackAction.Animated)
    }

    onStatusChanged: {
        if (status === PageStatus.Active && players.current && pageStack.nextPage(page) === null) {
            pageStack.pushAttached(Qt.resolvedUrl("HomePage.qml"))
        }
    }

    // Every player polls only while this list is on screen (spec: background polling).
    Binding {
        target: players
        property: "pollAll"
        value: page.status === PageStatus.Active && Qt.application.active
    }

    function _availabilityText(availability) {
        switch (availability) {
        case 0: return qsTr("Connecting")
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
                text: qsTr("Add player")
                onClicked: pageStack.push(Qt.resolvedUrl("AddPlayerDialog.qml"))
            }
            MenuItem {
                text: qsTr("Search again")
                onClicked: players.search()
            }
        }

        delegate: ListItem {
            id: item
            // A group is one row, its primary's (spec 005, canvas G3).
            readonly property bool _follower: model.leaderId !== ""
            readonly property bool _leads: model.memberNames.length > 0
            visible: !_follower
            contentHeight: _follower ? 0 : (_leads ? Theme.itemSizeLarge : Theme.itemSizeMedium)
            // Long press (spec 005, canvas A1): join the current player's
            // group, take a member out of this group, or remove a player
            // added by address.
            readonly property string _id: model.playerId
            readonly property bool _canJoin: model.canJoin && players.current !== null
                                             && !players.current.groupBusy
            menu: _canJoin || _leads || model.manual ? playerMenu : null
            highlighted: down || model.playerId === players.currentId

            onClicked: {
                players.currentId = model.playerId
                page.openCurrent(false)
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
                    text: item._leads ? qsTr("%1 + %2").arg(model.name).arg(model.memberNames.length) : model.name
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
                          : page._availabilityText(model.availability)
                }
                Label {
                    visible: item._leads
                    width: parent.width
                    font.pixelSize: Theme.fontSizeExtraSmall
                    truncationMode: TruncationMode.Fade
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    text: qsTr("With %1").arg(model.memberNames.join(", "))
                }
            }

            Component {
                id: playerMenu
                ContextMenu {
                    MenuItem {
                        visible: item._canJoin
                        text: qsTr("Join %1").arg(players.current ? players.current.name : "")
                        onClicked: players.join(model.playerId)
                    }
                    Repeater {
                        model: item._leads ? players.memberIds(item._id) : []
                        MenuItem {
                            readonly property int _row: players.indexOf(modelData)
                            text: qsTr("Ungroup %1").arg(_row >= 0 ? players.nameAt(_row) : modelData)
                            onClicked: players.leave(modelData)
                        }
                    }
                    MenuItem {
                        visible: model.manual
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
