# Feature specification — 003 MPRIS lock-screen controls

Status: revised 2026-11-08 after the scope/design review (approve with
changes; findings applied below). Open: Misja's answers to Q1 and Q2. No
implementation yet.

## Owner and linked issue

- Accountable owner: root (primary assistant session).
- Decision owner: Misja Hoebe (@misja).
- Issue: none yet. 001 lists 003 in its "Agreed structure for later
  features". An issue is opened once Misja approves.
- Review: a separate agent reviews scope and design before implementation,
  and the result before the PR. The permission change makes this
  release-relevant.
- UI: no page changes. The controls are the system's own lock-screen and
  events-view media controls, so no canvas is needed (see open question Q1).

## Problem

Swell shows what the current player plays, and controls it, only inside the
app and on its cover. The system media controls on the lock screen do not
show Swell, so pausing or skipping on the player means unlocking the phone.
Sailfish shows those controls for any app that publishes an MPRIS player on
the session bus.

## Facts (2026-11-08)

- **Amber.Mpris 1.2.9** is in the build target: amber-qml-plugin-mpris
  1.2.9-1.9.4.jolla, source https://github.com/sailfishos/amber-mpris.
  - The QML type is `MprisPlayer` (Amber.Mpris 1.0), from `plugins.qmltypes`.
  - Properties: serviceName, identity, desktopEntry, canControl, canPlay,
    canPause, canGoNext, canGoPrevious, canSeek, canRaise, canQuit,
    playbackStatus, position and metaData. metaData holds title,
    contributingArtist, albumTitle, artUrl, duration and others.
  - Signals: playRequested, pauseRequested, playPauseRequested,
    nextRequested, previousRequested, raiseRequested, seekRequested,
    positionRequested, …
- **Behaviour of amber-mpris 1.2.9 (tag a5fc15b), verified by the review:**
  - `position` and `metaData.duration` are in milliseconds, and D-Bus gets
    microseconds. A duration of 0 or less removes `mpris:length`.
  - The QML type adds the process id to the name:
    `org.mpris.MediaPlayer2.harbour-swell.instance<pid>`. That still matches
    the permission's `org.mpris.MediaPlayer2.*`. An empty serviceName still
    registers `….instance<pid>`, which is not a valid name, so serviceName
    is never bound to a condition.
  - When `canControl` is false, all can* properties read false and D-Bus
    calls are refused.
  - `canControl`, `hasShuffle` and `hasLoopStatus` lock after the first
    D-Bus GetAll or Introspect; later changes are silently ignored.
    `hasShuffle` and `hasLoopStatus` default to true.
  - Reading `position` emits `positionRequested` synchronously. Upstream's
    pattern is to assign `position` in that handler.
  - playbackStatus uses `Mpris.Stopped` (0), `Mpris.Playing` (1) and
    `Mpris.Paused` (2).
  - Amber's `MprisController` changes the current service only when none is
    current, or when a service starts Playing and the current one is not
    Playing (`mpriscontroller.cpp:975-1080`).
- **The lock screen (a community copy of Sailfish's QML; its version is not
  known):**
  - It shows the controls while any MPRIS service exists, also a Stopped one.
  - Play/pause calls PlayPause.
  - Art is a plain `Image` in lipstick, which is not sandboxed.
  - Lipstick also sends headset and media keys to the current service.
- **Harbour:**
  - `allowed_qmlimports.conf:44` allows `Amber.Mpris 1.0`.
  - `allowed_requires.conf:62-63` allows `amber-qml-plugin-mpris` and
    `qml(Amber.Mpris)`.
- **Sandbox:**
  - Only `Audio.permission` grants `dbus-user.own org.mpris.MediaPlayer2.*`.
    This holds in the target, on the phone (Sailfish OS 5.2.0.20) and in
    sailjail-permissions master (027ddfc). No narrower permission exists.
  - The same permission also opens PulseAudio, which can include recording.
    Its descriptions are "Play and record audio" (short) and "Play and record
    audio, and show audio controls on lockscreen" (long); which one the
    prompt shows is checked on the device.
  - Precedent: gPodder and harbour-sailcast publish MPRIS with
    Permissions=Audio. Both also play audio themselves; no sandboxed
    remote-control app was found that does this.
- **Packaging:** this target's rpm generates no `qml(...)` requires. The
  plugin package provides `qml(Amber.Mpris)`
  (`rpm -q --provides amber-qml-plugin-mpris`, 2026-11-08).
  - Swell has `Permissions=Internet` today.
- **What the client already offers:**
  - `state` (play, stream, pause, stop, …) and `playing`;
  - `title1..3` and `artworkUrl` (an http URL on the player, with
    followRedirects=1);
  - `duration` and `position` in seconds;
  - `canSkip` and `canBack`, and `play()`, `pause()`, `skip()` and `back()`.

  There is no seek.

## Scope

1. **`qml/Mpris.qml`:** an `MprisPlayer` with a `player` property, which
   `harbour-swell.qml` sets to `players.current`. It needs no C++ and no
   Silica.
   - **Constant properties,** as literals, because amber locks them:
     - `serviceName` "harbour-swell", `identity` "Swell" and `desktopEntry`
       "harbour-swell";
     - `canControl` true, `hasShuffle` false and `hasLoopStatus` false;
     - `canSeek` false and `canRaise` true.
   - **Playback status:** Playing when `player.playing` (play, stream); Paused
     when `state` is "pause"; Stopped otherwise, and when there is no player
     or it is offline.
   - **Capabilities:**
     - `canPlay` and `canPause` follow `player && player.online`;
     - `canGoNext` follows `canSkip` and `canGoPrevious` follows `canBack`,
       both also requiring online.
   - **Metadata:**
     - title ← `title1`, contributingArtist ← `title2` and albumTitle ←
       `title3`, as plain strings;
     - artUrl ← `String(artworkUrl)`;
     - duration ← `duration` × 1000.
     - All are cleared, not left stale, when there is no player or it is
       offline. The controls then still show "Swell", with disabled
       buttons.
   - **Position:** set in `onPositionRequested` to
     `max(0, player.position) × 1000`, since `position` is -1 when unknown.
     There is no binding and no Seeked signal.
   - **Requests:**
     - playPause uses `player.playing`, as the cover does;
     - play, pause, next and previous call the commands directly;
     - each handler first checks `player && player.online`;
     - stop, seek, shuffle and loop requests are ignored.
   - **Raise:** `harbour-swell.qml` handles it with `app.activate()`.
2. **Packaging:**
   - `Permissions=Internet;Audio` in the desktop file;
   - `Requires: qml(Amber.Mpris)` in the spec, as gPodder and sailcast do.
     No BuildRequires: the tests run in the SDK target, not in `%check`.
3. **Text:** one sentence on why Swell asks for Audio, in the RPM
   description and the changelog. About stays as it is, unless Misja wants
   it there too (Q2).

## Out of scope

- Volume through MPRIS. The lock screen does not offer it, and the volume
  keys stay the phone's.
- Seeking: Swell has no seek command.
- Publishing more than one player, a player per BluOS device, and track
  lists.
- A C++ MPRIS implementation (libambermpris is not an allowed library).
- Anything that keeps Swell running in the background beyond what it does
  today.

## Open questions for Misja

- **Q1, Swell takes the lock screen and the headset buttons.** While the
  BluOS player plays, Swell is "Playing" and owns the system media controls.
  - A local player that starts afterwards (Jolla Media, a podcast app) does
    not take them over.
  - Headset and Bluetooth play/pause/next then control the living-room
    player, not the phone.
  - The controls show only while Swell runs; there is no setting.

  Is that acceptable? There is no cheap way around it, and none is proposed
  before you decide.
- **Q2, the Audio permission.** On first start, and probably again on
  upgrade for current users, the sandbox asks for "Play and record audio
  (…and show audio controls on lockscreen)". Swell plays and records
  nothing; the permission is only for the controls. Refusing it probably
  stops Swell from starting. Both are inferred and checked on the device.
  Acceptable? Should About also explain it? (The alternative is no 003.)

## Acceptance criteria

1. While Swell runs and a player is current, the lock screen shows Swell's
   controls with title and artist. Play/pause, next and previous act on the
   player, and next and previous are disabled when the player cannot skip.
2. The controls follow a change of current player, and changes made in the
   official app or on the player itself.
3. With no player, or with the player offline, the controls show "Swell",
   stopped, with no title or art, and do nothing.
4. The validator passes with the new import, requirement and permission. The
   RPM requires `qml(Amber.Mpris)`.
5. All existing suites pass. The runtime QML test creates `Mpris.qml`
   against the fake player, with no session bus
   (`DBUS_SESSION_BUS_ADDRESS` pointing nowhere), and checks:
   - playbackStatus: play and stream are 1, pause is 2, stop and
     connecting are 0;
   - canGoNext and canGoPrevious follow a stream's skip and back actions,
     which also catches canControl not being true;
   - metaData title, contributingArtist, albumTitle, artUrl and duration;
   - position: secs × 1000, and 0 when unknown;
   - requests: playPause, next and previous reach `/Pause` or `/Play`,
     `/Skip` and `/Back`;
   - offline: canPlay and canPause are false, and the metadata is cleared.
6. A device check by Misja covers:
   - the lock screen and the events view, including cover art over http;
   - playing from the official app while the screen is locked;
   - the first-start permission prompt and an upgrade from 0.3.0-13;
   - local audio started while BluOS plays, and headset play/pause (Q1).
7. A separate result review before the PR.
