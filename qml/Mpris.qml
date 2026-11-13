// SPDX-License-Identifier: MIT
import QtQuick 2.0
import Amber.Mpris 1.0

// The current player on the system media controls: the lock screen, the
// events view and the headset keys (spec 003). Without the Audio permission
// the service is not registered and nothing else changes.
MprisPlayer {
    id: mpris

    // A BluOS client (Player), or null.
    property QtObject player
    // With nothing to play, every can* is false and the metadata empty, so
    // the system controls (Sailfish.Media MprisControls) hide themselves.
    readonly property bool _active: player !== null && player.online && player.hasContent

    // Constants, never bindings: amber locks canControl, hasShuffle and
    // hasLoopStatus after the first D-Bus GetAll or Introspect, and an empty
    // serviceName would still register an invalid name.
    serviceName: "harbour-swell"
    identity: "Swell"
    desktopEntry: "harbour-swell"
    canControl: true
    hasShuffle: false
    hasLoopStatus: false
    canSeek: false
    canRaise: true

    canPlay: _active
    canPause: _active
    canGoNext: _active && player.canSkip
    canGoPrevious: _active && player.canBack

    playbackStatus: {
        if (!_active) {
            return Mpris.Stopped
        }
        if (player.playing) {
            return Mpris.Playing
        }
        return player.state === "pause" ? Mpris.Paused : Mpris.Stopped
    }

    metaData.title: _active ? player.title1 : ""
    metaData.contributingArtist: _active ? player.title2 : ""
    metaData.albumTitle: _active ? player.title3 : ""
    metaData.artUrl: _active ? String(player.artworkUrl) : ""
    metaData.duration: _active && player.duration > 0 ? player.duration * 1000 : 0

    // Read on demand; the position is -1 when the player does not report one.
    onPositionRequested: position = _active ? Math.max(0, player.position) * 1000 : 0

    onPlayRequested: {
        if (_active) {
            player.play()
        }
    }
    onPauseRequested: {
        if (_active) {
            player.pause()
        }
    }
    onPlayPauseRequested: {
        if (_active) {
            if (player.playing) {
                player.pause()
            } else {
                player.play()
            }
        }
    }
    onNextRequested: {
        if (_active && player.canSkip) {
            player.skip()
        }
    }
    onPreviousRequested: {
        if (_active && player.canBack) {
            player.back()
        }
    }
}
