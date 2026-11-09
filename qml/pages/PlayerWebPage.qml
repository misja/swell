// SPDX-License-Identifier: MIT
// A page the player itself serves, in the Sailfish WebView (spec 006): its
// update page for now. Swell sends nothing to it; the player's page does.
import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.WebView 1.0

WebViewPage {
    id: page

    property url url
    property string title
    property string description

    allowedOrientations: Orientation.All

    WebViewFlickable {
        anchors.fill: parent
        header: PageHeader {
            title: page.title
            description: page.description
        }
        webView.privateMode: true
        webView.url: page.url
    }
}
