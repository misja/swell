// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0
import Swell 1.0

// The current player's group (spec 005, canvas G2): the group volume, the
// members with their own volume, and the players that can join. A switch
// adds a player or takes it out; the current player leads.
Page {
    id: page
    objectName: "group"

    readonly property QtObject _player: players.current
    readonly property bool _busy: _player !== null && _player.groupBusy
    // The current player follows a primary Swell does not know: it cannot
    // lead here (spec 005: offer to add the primary instead).
    readonly property bool _follows: _player !== null && _player.groupRole === Player.Secondary
    // The members as the current player lists them; re-read when that changes.
    readonly property var _memberIds: _player && _player.secondaryIds ? players.memberIds(players.currentId) : []
    // Members the model does not know (not found, not added): by address.
    readonly property var _unknown: {
        var list = []
        for (var i = 0; i < _memberIds.length; ++i) {
            if (players.indexOf(_memberIds[i]) < 0) {
                list.push(_memberIds[i])
            }
        }
        return list
    }

    allowedOrientations: Orientation.All

    // The other players are kept current while this page is shown.
    onStatusChanged: players.groupPage = status === PageStatus.Activating || status === PageStatus.Active
    Component.onDestruction: players.groupPage = false

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTr("Group")
                description: _player && !page._follows ? qsTr("%1 leads").arg(_player.name !== "" ? _player.name : _player.host) : ""
            }

            Label {
                visible: page._follows
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                text: qsTr("This player follows %1, which is not in your player list.").arg(_player ? _player.primaryId : "")
            }
            Button {
                visible: page._follows
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Add %1").arg(_player ? _player.primaryId : "")
                onClicked: players.addManual(_player.primaryId)
            }

            Slider {
                id: groupVolume
                width: parent.width
                visible: _player !== null && _player.groupRole === Player.Primary
                         && (_player.groupVolume >= 0 || _player.volume >= 0)
                minimumValue: 0
                maximumValue: 100
                stepSize: 1
                label: qsTr("Group volume")
                valueText: Math.round(value)
                // As on Now Playing: sent while dragging; the client keeps
                // only the latest level per second.
                onValueChanged: {
                    if (down && _player) {
                        _player.setGroupVolume(Math.round(value))
                    }
                }

                Binding {
                    target: groupVolume
                    property: "value"
                    value: _player ? (_player.groupVolume >= 0 ? _player.groupVolume : Math.max(0, _player.volume)) : 0
                    when: !groupVolume.down
                }
            }

            SectionHeader {
                visible: !page._follows
                text: qsTr("In this group")
            }

            Repeater {
                model: players
                delegate: Column {
                    readonly property bool _lead: model.playerId === players.currentId
                    readonly property bool _member: page._memberIds.indexOf(model.playerId) >= 0
                    width: column.width
                    visible: (_lead && !page._follows) || _member

                    TextSwitch {
                        text: model.name
                        description: _lead ? qsTr("Leads") : ""
                        checked: true
                        enabled: _member && !page._busy
                        busy: page._busy && _member
                        automaticCheck: false
                        onClicked: players.leave(model.playerId)
                    }
                    Slider {
                        id: memberVolume
                        width: parent.width
                        visible: model.volume >= 0
                        minimumValue: 0
                        maximumValue: 100
                        stepSize: 1
                        valueText: Math.round(value)
                        onValueChanged: {
                            if (down) {
                                players.setPlayerVolume(model.playerId, Math.round(value))
                            }
                        }

                        Binding {
                            target: memberVolume
                            property: "value"
                            value: model.volume
                            when: !memberVolume.down
                        }
                    }
                }
            }

            Repeater {
                model: page._unknown
                delegate: TextSwitch {
                    text: modelData
                    description: qsTr("Not in your player list")
                    checked: true
                    enabled: !page._busy
                    automaticCheck: false
                    onClicked: players.leave(modelData)
                }
            }

            SectionHeader {
                visible: !page._follows
                text: qsTr("Can join")
            }

            Repeater {
                model: players
                // Only lone players: one already in a group is changed from there.
                delegate: TextSwitch {
                    visible: !page._follows && model.playerId !== players.currentId
                             && page._memberIds.indexOf(model.playerId) < 0
                             && model.online && model.groupRole === Player.Alone
                    text: model.name
                    description: model.nowPlaying
                    checked: false
                    enabled: !page._busy
                    automaticCheck: false
                    onClicked: players.join(model.playerId)
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
