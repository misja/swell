// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0

// The content of one queue entry: artwork, title, "artist · album" and the
// duration. Used by the Queue page's rows and by the row lifted while
// reordering.
Item {
    id: row

    property bool loaded
    property string title
    property string artist
    property string album
    property int duration
    property url image
    // The current entry while an input or stream plays: dimmed (spec 002).
    property bool dimmed
    property bool highlighted

    CoverImage {
        id: art
        x: Theme.horizontalPageMargin
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.itemSizeMedium - 2 * Theme.paddingSmall
        height: width
        highlighted: row.highlighted
        source: row.image
    }

    Column {
        anchors {
            left: art.right
            leftMargin: Theme.paddingLarge
            right: time.left
            rightMargin: Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }

        Label {
            width: parent.width
            truncationMode: TruncationMode.Fade
            color: row.highlighted ? Theme.highlightColor : Theme.primaryColor
            opacity: row.dimmed ? Theme.opacityHigh : 1.0
            text: row.loaded ? row.title : "…"
        }
        Label {
            width: parent.width
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: row.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
            text: {
                if (!row.loaded) {
                    return ""
                }
                if (row.artist !== "" && row.album !== "") {
                    return row.artist + " · " + row.album
                }
                return row.artist + row.album
            }
        }
    }

    Label {
        id: time
        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        font.pixelSize: Theme.fontSizeExtraSmall
        color: row.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
        text: row.loaded ? app.formatTime(row.duration) : ""
    }
}
