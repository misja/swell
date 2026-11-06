// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0
import Swell 1.0

// One level of the player's /Browse hierarchy (spec 004). The top level, every
// source, letter lists, albums and search results all use this page.
Page {
    id: page

    property bool root
    property string key
    property string query
    property string title
    // The row that opened this page, for an album header and its Shuffle.
    property url headerImage
    property string headerSubtitle
    property var parentActions: []
    // The searchKey of the nearest page above that has one.
    property string inheritedSearchKey
    // Opened from an artist row: show its albums at once (canvas B8).
    property bool expandFirst
    // Opened from an album row: its tracks get the album header (B5).
    property bool fromAlbum

    readonly property QtObject _player: players.current
    readonly property bool _tracks: browseModel.listType === "tracks" && fromAlbum
    // Covers in every list except menus and an album's own tracks (B9).
    readonly property bool _images: !root && browseModel.listType !== "menu" && !_tracks
    readonly property bool _favouriteAlbums: browseModel.favouritesKey.indexOf("LocalMusic:Album/") === 0
    readonly property bool _offline: _player !== null && !_player.online && (!browseModel.ready || browseModel.error !== "")
    readonly property string _searchKey: browseModel.searchKey !== "" ? browseModel.searchKey : inheritedSearchKey

    allowedOrientations: Orientation.All

    function _actionText(type) {
        switch (type) {
        case "add-now":
        case "addAll-now":
            return qsTr("Play now")
        case "add-shuffle":
            return qsTr("Shuffle")
        case "add-next":
        case "addAll-next":
            return qsTr("Add next")
        case "add-last":
        case "addAll-last":
            return qsTr("Add last")
        case "favourite-add":
            return qsTr("Add to favourites")
        case "favourite-delete":
            return qsTr("Remove from favourites")
        }
        return ""
    }

    function _formatTime(secs) {
        if (!(secs >= 0)) {
            return ""
        }
        var m = Math.floor(secs / 60)
        var s = secs % 60
        return m + ":" + (s < 10 ? "0" : "") + s
    }

    function _open(row) {
        pageStack.push(Qt.resolvedUrl("BrowsePage.qml"), {
            key: row.browseKey,
            title: row.favourites ? qsTr("Favourites") : row.text,
            headerImage: row.image,
            headerSubtitle: row.text2,
            parentActions: row.actions,
            inheritedSearchKey: page._searchKey,
            expandFirst: row.type === "artist",
            fromAlbum: row.type === "album"
        })
    }

    function _shuffleUrl() {
        for (var i = 0; i < parentActions.length; ++i) {
            if (parentActions[i].type === "add-shuffle") {
                return parentActions[i].url
            }
        }
        return ""
    }

    BrowseModel {
        id: browseModel
        client: page._player
        key: page.key
        query: page.query
        root: page.root
        expandFirst: page.expandFirst
        Component.onCompleted: load()
    }

    // The favourites of a library letter list, shown on top of it (B3/B4).
    BrowseModel {
        id: favouritesModel
        client: page._player
        key: browseModel.favouritesKey
        onKeyChanged: if (key !== "") load()
    }
    readonly property bool _hasFavourites: browseModel.favouritesKey !== "" && favouritesModel.ready
                                           && favouritesModel.count > 0

    // Only the page in front reacts: a favourite change must not reload (and
    // reset) the pages below it, and Home already reports failures.
    Connections {
        target: page.status === PageStatus.Active ? page._player : null
        onActionDone: {
            if (type.indexOf("favourite") === 0) {
                app.notify(type === "favourite-add" ? qsTr("Added to favourites") : qsTr("Removed from favourites"))
                browseModel.load()   // the menu label, and the Favourites list, change
                if (favouritesModel.key !== "") {
                    favouritesModel.load()
                }
            } else if (type.indexOf("-now") > 0 || type === "add-shuffle") {
                app.notify(qsTr("Playing"))
            } else {
                app.notify(qsTr("Added to the queue"))
            }
        }
    }

    // Read again when the player is back, if the first read did not succeed.
    Connections {
        target: page._player
        onAvailabilityChanged: {
            if (page._player.online && (browseModel.error !== "" || !browseModel.ready)) {
                browseModel.load()
            }
        }
    }

    SilicaListView {
        id: list
        anchors.fill: parent
        model: browseModel
        quickScroll: !browseModel.sectioned

        header: Column {
            width: list.width

            PageHeader {
                id: pageHeader
                visible: !page._tracks
                title: page.root ? qsTr("Browse") : page.title
                description: {
                    if (page.root) {
                        return page._player ? (page._player.name !== "" ? page._player.name : page._player.host) : ""
                    }
                    if (page.headerSubtitle !== "" && !page.fromAlbum) {
                        return page.headerSubtitle
                    }
                    return browseModel.serviceName !== page.title ? browseModel.serviceName : ""
                }
            }

            // An album: the cover beside the title (canvas B5).
            Row {
                id: albumHeader
                visible: page._tracks
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: cover.height + 2 * Theme.paddingLarge
                spacing: Theme.paddingLarge

                Image {
                    id: cover
                    y: Theme.paddingLarge
                    width: Theme.itemSizeExtraLarge
                    height: width
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    sourceSize.width: width
                    sourceSize.height: height
                    source: page.headerImage
                }

                Column {
                    width: parent.width - cover.width - parent.spacing
                    anchors.verticalCenter: cover.verticalCenter

                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignRight
                        wrapMode: Text.Wrap
                        maximumLineCount: 2
                        truncationMode: TruncationMode.Elide
                        font.pixelSize: Theme.fontSizeLarge
                        font.family: Theme.fontFamilyHeading
                        color: Theme.highlightColor
                        text: page.title
                    }
                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignRight
                        truncationMode: TruncationMode.Fade
                        color: Theme.secondaryHighlightColor
                        text: page.headerSubtitle
                    }
                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignRight
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                        text: browseModel.ready ? qsTr("%n tracks", "", list.count) : ""
                    }
                }
            }

            // Favourites on top of a library letter list, as in People:
            // a cover grid for albums, rows otherwise.
            Column {
                width: parent.width
                visible: page._hasFavourites

                SectionHeader {
                    text: qsTr("Favourites")
                }

                Grid {
                    id: coverGrid
                    readonly property int cell: Math.floor(width / columns)
                    // Clear of the letter index on the right.
                    width: parent.width - (browseModel.sectioned ? Theme.itemSizeExtraSmall : 0)
                    columns: page.isLandscape ? 6 : 3
                    visible: page._favouriteAlbums

                    Repeater {
                        id: favouriteCovers
                        model: page._favouriteAlbums ? favouritesModel : null
                        BackgroundItem {
                            width: coverGrid.cell
                            height: coverGrid.cell
                            onClicked: page._open(model)

                            Image {
                                anchors.fill: parent
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                sourceSize.width: width
                                sourceSize.height: height
                                source: model.image
                            }
                            // The image covers BackgroundItem's own highlight.
                            Rectangle {
                                anchors.fill: parent
                                color: Theme.highlightBackgroundColor
                                opacity: parent.highlighted ? Theme.highlightBackgroundOpacity : 0
                            }
                        }
                    }
                }

                Repeater {
                    id: favouriteRows
                    model: page._favouriteAlbums ? null : favouritesModel
                    ListItem {
                        contentHeight: Theme.itemSizeSmall
                        onClicked: page._open(model)
                        Label {
                            x: Theme.horizontalPageMargin
                            width: parent.width - Theme.horizontalPageMargin - Theme.itemSizeExtraSmall
                            anchors.verticalCenter: parent.verticalCenter
                            truncationMode: TruncationMode.Fade
                            text: model.text
                        }
                    }
                }
            }
        }

        PullDownMenu {
            visible: page._searchKey !== "" || page.root || browseModel.moreLinks.length > 0
                     || (browseModel.listType === "tracks" && browseModel.playUrl !== "")

            // Other views of an artist (its songs), B8.
            Repeater {
                model: browseModel.moreLinks
                MenuItem {
                    text: modelData.text
                    onClicked: page._open({ browseKey: modelData.browseKey, text: modelData.text,
                                            text2: page.title, image: "", actions: [],
                                            type: "link", favourites: false })
                }
            }

            MenuItem {
                text: qsTr("Search")
                visible: page._searchKey !== "" || page.root
                onClicked: pageStack.push(Qt.resolvedUrl("SearchPage.qml"), {
                    searchKey: page.root ? "" : page._searchKey,
                    sourceName: page.root ? "" : browseModel.serviceName
                })
            }
            MenuItem {
                text: qsTr("Shuffle")
                visible: page._tracks && page._shuffleUrl() !== ""
                onClicked: page._player.runAction(page._shuffleUrl(), "add-shuffle")
            }
            MenuItem {
                text: page._tracks ? qsTr("Play album") : qsTr("Play all")
                visible: browseModel.listType === "tracks" && browseModel.playUrl !== ""
                onClicked: page._player.playPath(browseModel.playUrl)
            }
        }

        // No header per letter (Misja: it clutters the list); the letter
        // index shows where the list is.

        delegate: ListItem {
            id: item

            readonly property var _actions: model.actions

            // Unloaded and loaded rows have the same height, so jumps land.
            // A link to a further page (TuneIn's "Sport", …) gets an icon-sized
            // image on a normal row; albums, tracks and stations a cover.
            // Letter lists hold one kind of row, so their heights stay equal.
            readonly property bool _cover: page._images && model.type !== "link"
            contentHeight: _cover ? Theme.itemSizeMedium : Theme.itemSizeSmall
            enabled: model.loaded
            menu: model.actions.length > 0 ? contextMenu : null

            onClicked: {
                if (model.browseKey !== "") {
                    page._open(model)
                } else if (model.autoplayUrl !== "") {
                    page._player.playPath(model.autoplayUrl)
                } else if (model.playUrl !== "") {
                    page._player.playPath(model.playUrl)
                }
            }

            Icon {
                id: favouriteIcon
                visible: model.favourites
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                source: "image://theme/icon-m-favorite-selected"
                color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
            }

            Image {
                id: art
                visible: page._images && model.image.toString() !== ""
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                width: !visible ? 0 : (item._cover ? Theme.itemSizeMedium - 2 * Theme.paddingSmall : Theme.iconSizeMedium)
                height: width
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                sourceSize.width: width
                sourceSize.height: height
                source: visible ? model.image : ""
            }

            Column {
                anchors {
                    left: model.favourites ? favouriteIcon.right : (art.visible ? art.right : parent.left)
                    leftMargin: model.favourites || art.visible ? Theme.paddingLarge : Theme.horizontalPageMargin
                    right: duration.left
                    rightMargin: duration.text !== "" ? Theme.paddingMedium : 0
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                    text: model.favourites ? qsTr("Favourites") : (model.loaded ? model.text : "…")
                }
                Label {
                    width: parent.width
                    visible: text !== ""
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    text: model.loaded && model.type !== "section" ? model.text2 : ""
                }
            }

            Label {
                id: duration
                anchors {
                    right: parent.right
                    rightMargin: browseModel.sectioned ? Theme.itemSizeExtraSmall : Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                font.pixelSize: Theme.fontSizeExtraSmall
                color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                text: model.loaded ? page._formatTime(model.duration) : ""
            }

            Component {
                id: contextMenu
                ContextMenu {
                    Repeater {
                        model: item._actions
                        MenuItem {
                            text: page._actionText(modelData.type)
                            onClicked: page._player.runAction(modelData.url, modelData.type)
                        }
                    }
                }
            }
        }

        ViewPlaceholder {
            enabled: (browseModel.ready && list.count === 0) || page._offline
            text: {
                if (page._offline) {
                    return qsTr("Player offline")
                }
                if (browseModel.error !== "") {
                    return qsTr("The player could not show this")
                }
                return page.key === "LocalMusic:MG/LocalMusic-Favourites" ? qsTr("No favourites yet")
                                                                         : qsTr("Nothing here")
            }
            hintText: {
                if (page._offline) {
                    return qsTr("Swell will try again when it is back")
                }
                if (browseModel.error !== "") {
                    // The player's own words, or Swell's (translated).
                    return browseModel.playerError ? browseModel.error : qsTr("No usable answer from the player")
                }
                return page.key === "LocalMusic:MG/LocalMusic-Favourites"
                        ? qsTr("Long press an album, artist or track and choose Add to favourites") : ""
            }
        }

        BusyIndicator {
            anchors.centerIn: parent
            size: BusySize.Large
            running: !browseModel.ready && !page._offline
        }

        VerticalScrollDecorator {}
    }

    LetterIndex {
        visible: browseModel.sectioned
        anchors {
            right: parent.right
            top: parent.top
            topMargin: Theme.itemSizeLarge
            bottom: parent.bottom
            bottomMargin: Theme.paddingLarge
        }
        letters: browseModel.letters
        favourites: page._hasFavourites
        // The letter of the part in view stays lit, as in People.
        current: {
            list.contentY   // re-evaluate while scrolling
            var row = list.indexAt(0, list.contentY + Theme.itemSizeSmall)
            return row < 0 ? "" : browseModel.sectionAt(row)
        }
        onJump: list.positionViewAtIndex(row, ListView.Beginning)
        onTop: list.positionViewAtBeginning()
    }
}
