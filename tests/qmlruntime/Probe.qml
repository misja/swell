// SPDX-License-Identifier: MIT
// The expressions HomePage, BrowsePage and LetterIndex use on the models,
// each returned as one string so the test compares plain text.
import QtQuick 2.0
import Swell 1.0
import "../../qml/pages"

Item {
    id: probe

    readonly property QtObject _player: players.current

    // HomePage: presets and inputs
    readonly property int presetCount: _player ? _player.presets.length : -1
    readonly property int inputCount: _player ? _player.inputs.length : -1

    function presetAt(i) {
        var p = _player.presets[i]
        return [p.id, p.name, String(p.image), p.inputId].join("|")
    }
    function inputAt(i) {
        var p = _player.inputs[i]
        return [p.id, p.text, String(p.image), p.url].join("|")
    }
    // HomePage._inputImage
    function inputImage(inputId) {
        for (var i = 0; i < _player.inputs.length; ++i) {
            if (_player.inputs[i].id === inputId) {
                return String(_player.inputs[i].image)
            }
        }
        return ""
    }
    function presetTile(i) {
        return presetTiles.itemAt(i).keys
    }
    function inputTile(i) {
        return inputTiles.itemAt(i).keys
    }

    Repeater {
        id: presetTiles
        model: probe._player ? probe._player.presets : []
        Item {
            readonly property string keys: [modelData.id, modelData.name, String(modelData.image),
                                            modelData.inputId].join("|")
        }
    }
    Repeater {
        id: inputTiles
        model: probe._player ? probe._player.inputs : []
        Item {
            readonly property string keys: [modelData.id, modelData.text, String(modelData.image),
                                            modelData.url].join("|")
        }
    }

    // BrowsePage: rows and their context menus
    BrowseModel {
        id: listModel
        client: probe._player
        key: "List"
    }
    Repeater {
        id: rows
        model: listModel
        Item {
            readonly property var actions: model.actions
        }
    }
    function rowActions(i) {
        var a = rows.itemAt(i).actions
        var list = []
        for (var j = 0; j < a.length; ++j) {
            list.push([a[j].type, a[j].text, a[j].url].join(" "))
        }
        return a.length + ":" + list.join("|")
    }
    // _open(row) pushes the next page with parentActions: row.actions,
    // which _shuffleUrl reads there.
    property var parentActions: []
    function openRow(i) {
        parentActions = rows.itemAt(i).actions
    }
    function shuffleUrl() {
        for (var i = 0; i < parentActions.length; ++i) {
            if (parentActions[i].type === "add-shuffle") {
                return parentActions[i].url
            }
        }
        return ""
    }

    // The favourites row on the top level, and a row not read yet.
    BrowseModel {
        id: rootModel
        client: probe._player
        root: true
    }
    function rootRow(i) {
        return rootModel.data(rootModel.index(i, 0), browseRoles.favourites) + " "
                + String(rootModel.data(rootModel.index(i, 0), browseRoles.actions))
    }
    BrowseModel {
        id: indexModel
        client: probe._player
        key: "Index"
    }
    function unreadActions(row) {
        var a = indexModel.data(indexModel.index(row, 0), browseRoles.actions)
        return indexModel.data(indexModel.index(row, 0), browseRoles.loaded) + " " + a.length
    }

    // LetterIndex, the real component.
    LetterIndex {
        id: letterIndex
        letters: indexModel.letters
    }
    function letters() {
        var list = []
        for (var i = 0; i < letterIndex._letters.length; ++i) {
            list.push(letterIndex._letters[i].text + letterIndex._letters[i].row)
        }
        return list.join(" ")
    }

    // BrowsePage: an artist opens on its albums; its other links.
    BrowseModel {
        id: artistModel
        client: probe._player
        key: "Artist:X"
        expandFirst: true
    }
    function moreLinks() {
        var l = artistModel.moreLinks
        var list = []
        for (var i = 0; i < l.length; ++i) {
            list.push(l[i].text + " " + l[i].browseKey)
        }
        return l.length + ":" + list.join("|") + ":" + (linkButtons.count > 0 ? linkButtons.itemAt(0).keys : "")
    }
    Repeater {
        id: linkButtons
        model: artistModel.moreLinks
        Item {
            readonly property string keys: modelData.text + " " + modelData.browseKey
        }
    }

    readonly property bool ready: listModel.ready && rootModel.ready && indexModel.ready && artistModel.ready
    function loadAll() {
        listModel.load()
        rootModel.load()
        indexModel.load()
        artistModel.load()
    }
}
