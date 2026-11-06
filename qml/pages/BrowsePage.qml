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

    readonly property QtObject _player: players.current
    readonly property bool _tracks: browseModel.listType === "tracks"
    readonly property bool _images: browseModel.listType !== "menu" && browseModel.listType !== "tracks"
                                   && browseModel.listType !== "sections" && !root
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
        if (secs < 0) {
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
            inheritedSearchKey: page._searchKey
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
        Component.onCompleted: load()
    }

    Connections {
        target: page._player
        onActionDone: {
            if (type.indexOf("favourite") === 0) {
                app.notify(type === "favourite-add" ? qsTr("Added to favourites") : qsTr("Removed from favourites"))
                browseModel.load()   // the menu label, and the Favourites list, change
            } else if (type.indexOf("-now") > 0 || type === "add-shuffle") {
                app.notify(qsTr("Playing"))
            } else {
                app.notify(qsTr("Added to the queue"))
            }
        }
        onCommandFailed: {
            console.warn("Command", path, "failed:", error)
            app.notify(qsTr("The player did not accept that."))
        }
    }

    SilicaListView {
        id: list
        anchors.fill: parent
        model: browseModel
        quickScroll: !browseModel.sectioned

        header: Item {
            width: list.width
            height: page._tracks ? albumHeader.height : pageHeader.height

            PageHeader {
                id: pageHeader
                visible: !page._tracks
                title: page.root ? qsTr("Browse") : page.title
                description: {
                    if (page.root) {
                        return page._player ? (page._player.name !== "" ? page._player.name : page._player.host) : ""
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
        }

        PullDownMenu {
            visible: page._searchKey !== "" || page.root || (page._tracks && browseModel.playUrl !== "")

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
                text: qsTr("Play album")
                visible: page._tracks && browseModel.playUrl !== ""
                onClicked: page._player.playPath(browseModel.playUrl)
            }
        }

        section.property: browseModel.sectioned ? "section" : ""
        section.delegate: SectionHeader {
            text: section
        }

        delegate: ListItem {
            id: item

            readonly property var _actions: model.actions

            // Unloaded and loaded rows have the same height, so jumps land.
            contentHeight: page._images ? Theme.itemSizeMedium : Theme.itemSizeSmall
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
                visible: page._images
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                width: visible ? Theme.itemSizeMedium - 2 * Theme.paddingSmall : 0
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
            enabled: browseModel.ready && list.count === 0
            text: {
                if (browseModel.error !== "") {
                    return qsTr("The player could not show this")
                }
                return page.key === "LocalMusic:MG/LocalMusic-Favourites" ? qsTr("No favourites yet")
                                                                         : qsTr("Nothing here")
            }
            hintText: {
                if (browseModel.error !== "") {
                    return browseModel.error
                }
                return page.key === "LocalMusic:MG/LocalMusic-Favourites"
                        ? qsTr("Long press an album, artist or track and choose Add to favourites") : ""
            }
        }

        BusyIndicator {
            anchors.centerIn: parent
            size: BusySize.Large
            running: !browseModel.ready
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
        onJump: list.positionViewAtIndex(row, ListView.Beginning)
    }
}
