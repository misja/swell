// SPDX-License-Identifier: MIT
// Confirms a firmware update before Swell starts it (spec 006, revision
// 2026-11-25). Once the player has it, the update cannot be stopped.
import QtQuick 2.0
import Sailfish.Silica 1.0
import Swell 1.0

Dialog {
    id: dialog

    // The player this dialog was opened for, and the version it names.
    property QtObject player
    property string version

    allowedOrientations: Orientation.All
    canAccept: player !== null && player === players.current && player.online
               && player.upgradeAvailable && player.upgradeVersion === version
               && !player.upgradeStarted && player.upgradePhase !== Player.UpgradeChecking

    onAccepted: player.startUpgrade(version)

    Column {
        width: parent.width

        // Accept and cancel keep the platform's own texts.
        DialogHeader {
            title: dialog.version !== "" ? qsTr("Update to firmware %1").arg(dialog.version)
                                         : qsTr("Firmware update")
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            wrapMode: Text.Wrap
            color: Theme.highlightColor
            text: qsTr("The player restarts and cannot be used for a few minutes. The update cannot be stopped. Keep the player switched on.")
        }
    }
}
