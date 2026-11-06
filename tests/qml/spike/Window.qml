// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import Swell 1.0

ApplicationWindow {
    ConfigurationValue {
        key: "/io/github/misja/harbour-swell/test"
        defaultValue: ""
    }
    initialPage: Component {
        Page {
            allowedOrientations: Orientation.All
            SilicaListView {
                anchors.fill: parent
                header: PageHeader { title: "Spike" }
                PullDownMenu { MenuItem { text: "About" } }
            }
        }
    }
}
