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
    // Members the model does not know (not found, not added): by address.
    property var _unknown: []

    allowedOrientations: Orientation.All

    function _refreshUnknown() {
        var list = []
        var ids = players.memberIds(players.currentId)
        for (var i = 0; i < ids.length; ++i) {
            if (players.indexOf(ids[i]) < 0) {
                list.push(ids[i])
            }
        }
        _unknown = list
    }

    onStatusChanged: {
        if (status === PageStatus.Activating) {
            players.groupPage = true
            _refreshUnknown()
        } else if (status === PageStatus.Deactivating) {
            players.groupPage = false
        }
    }

    Connections {
        target: page._player
        onSyncStatusChanged: page._refreshUnknown()
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTr("Group")
                description: _player ? qsTr("%1 leads").arg(_player.name !== "" ? _player.name : _player.host) : ""
            }

            Slider {
                id: groupVolume
                width: parent.width
                visible: _player !== null && _player.groupRole === Player.Primary
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

            SectionHeader { text: qsTr("In this group") }

            Repeater {
                model: players
                delegate: Column {
                    readonly property bool _lead: model.playerId === players.currentId
                    readonly property bool _member: model.leaderId === players.currentId
                    width: column.width
                    visible: _lead || _member

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

            SectionHeader { text: qsTr("Can join") }

            Repeater {
                model: players
                delegate: TextSwitch {
                    visible: model.playerId !== players.currentId && model.leaderId !== players.currentId
                             && model.online
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
