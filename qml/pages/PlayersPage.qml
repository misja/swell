// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0
import Swell 1.0

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

        // A group is a small tree: its primary, then its members indented
        // (spec 005, canvas A1). Members' own rows are hidden; they are drawn
        // under their primary instead.
        delegate: Column {
            id: entry
            readonly property string _id: model.playerId
            readonly property bool _follower: model.leaderId !== ""
            readonly property bool _leads: model.memberNames.length > 0
            // Re-read when the member names change.
            readonly property var _members: _leads ? (model.memberNames, players.memberIds(_id)) : []
            readonly property var _names: model.memberNames
            width: list.width
            visible: !_follower

            ListItem {
                id: item
                contentHeight: entry._follower ? 0 : Theme.itemSizeMedium
                // Long press: join the current player's group, or remove a
                // player added by address.
                readonly property bool _canJoin: model.canJoin && players.current !== null
                                                 && !players.current.groupBusy
                // Follows a primary Swell does not know: offer to add it.
                readonly property bool _unknownLeader: model.groupRole === Player.Secondary
                                                       && model.leaderId === "" && model.primaryId !== ""
                menu: _canJoin || _unknownLeader || model.manual ? playerMenu : null
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
                              : page._availabilityText(model.availability)
                    }
                }

                Component {
                    id: playerMenu
                    ContextMenu {
                        MenuItem {
                            visible: item._canJoin
                            text: qsTr("Follow %1").arg(players.current ? players.current.name : "")
                            onClicked: players.join(entry._id)
                        }
                        MenuItem {
                            visible: item._unknownLeader
                            text: qsTr("Add %1").arg(model.primaryId)
                            onClicked: players.addManual(model.primaryId)
                        }
                        MenuItem {
                            visible: model.manual
                            text: qsTr("Remove")
                            onClicked: players.removeManual(entry._id)
                        }
                    }
                }
            }

            Repeater {
                model: entry._members
                delegate: ListItem {
                    id: member
                    // In the same order as the ids, and updated with the model.
                    readonly property string _name: entry._names.length > index ? entry._names[index] : modelData
                    contentHeight: Theme.itemSizeSmall
                    highlighted: down || entry._id === players.currentId
                    menu: memberMenu

                    // A member plays its primary's source: choosing it
                    // chooses the primary.
                    onClicked: {
                        players.currentId = entry._id
                        page.openCurrent(false)
                    }

                    Rectangle {
                        id: branch
                        x: Theme.horizontalPageMargin + Theme.paddingLarge
                        width: Theme.paddingSmall / 2
                        height: parent.height - Theme.paddingSmall
                        anchors.verticalCenter: parent.verticalCenter
                        color: Theme.highlightColor
                        opacity: Theme.opacityLow
                    }
                    Label {
                        anchors {
                            left: branch.right
                            leftMargin: Theme.paddingLarge
                            right: parent.right
                            rightMargin: Theme.horizontalPageMargin
                            verticalCenter: parent.verticalCenter
                        }
                        text: member._name
                        truncationMode: TruncationMode.Fade
                        color: member.highlighted ? Theme.highlightColor : Theme.primaryColor
                    }

                    Component {
                        id: memberMenu
                        ContextMenu {
                            MenuItem {
                                text: qsTr("Leave group")
                                enabled: players.current === null || !players.current.groupBusy
                                onClicked: players.leave(modelData, entry._id)
                            }
                        }
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
