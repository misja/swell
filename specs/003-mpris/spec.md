# Feature specification — 003 MPRIS lock-screen controls

Status: draft 2026-11-08, for the scope/design review. No implementation yet.

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
- **Units and names, from upstream source (master; to be checked against
  1.2.9 in the review):**
  - `position` and `metaData.duration` are in milliseconds; D-Bus gets
    microseconds.
  - `serviceName` becomes `org.mpris.MediaPlayer2.<name>`.
  - When `canControl` is false, all can* properties read false and D-Bus
    calls are refused.
- **Harbour:**
  - `allowed_qmlimports.conf:44` allows `Amber.Mpris 1.0`.
  - `allowed_requires.conf:62-63` allows `amber-qml-plugin-mpris` and
    `qml(Amber.Mpris)`.
- **Sandbox:**
  - Only `Audio.permission` grants `dbus-user.own org.mpris.MediaPlayer2.*`.
    This holds in the target and on the phone (Sailfish OS 5.2.0.20); no
    other permission grants it.
  - The same permission also opens PulseAudio, which can include recording.
    The user sees "Play and record audio, and show audio controls on
    lockscreen".
  - Swell has `Permissions=Internet` today.
- **What the client already offers:**
  - `state` (play, stream, pause, stop, …) and `playing`;
  - `title1..3` and `artworkUrl` (an http URL on the player, with
    followRedirects=1);
  - `duration` and `position` in seconds;
  - `canSkip` and `canBack`, and `play()`, `pause()`, `skip()` and `back()`.

  There is no seek.

## Scope

1. One `MprisPlayer` in the app window (QML only; no C++). It follows
   `players.current`:
   - **Identity:** `serviceName` "harbour-swell", `identity` "Swell",
     `desktopEntry` "harbour-swell".
   - **Playback status:** Playing when `playing`, Paused when `state` is
     "pause", Stopped otherwise. It is also Stopped when there is no current
     player or the player is offline.
   - **Capabilities:**
     - `canControl` is true;
     - `canPlay` and `canPause` follow `online`;
     - `canGoNext` follows `canSkip` and `canGoPrevious` follows `canBack`;
     - `canSeek` is false;
     - `canRaise` is true, and `raiseRequested` brings the app forward.
   - **Metadata:**
     - title ← `title1`;
     - contributingArtist ← `title2`;
     - albumTitle ← `title3`;
     - artUrl ← `artworkUrl`;
     - duration ← `duration` × 1000;
     - position ← `position` × 1000.
   - **Requests:** play, pause, playPause, next and previous call the
     player's commands. Seek requests are ignored.
2. **Packaging:**
   - `Permissions=Internet;Audio` in the desktop file;
   - `Requires: amber-qml-plugin-mpris` in the spec.
3. **About:** the privacy text says why Swell asks for Audio, if Misja agrees
   (Q2).

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

- **Q1:** is the system's own lock-screen control enough, with no setting to
  turn it off? (Recommended: yes. It shows only while Swell runs.)
- **Q2:** the Audio permission. On first start, the sandbox will ask for
  "Play and record audio, and show audio controls on lockscreen". Swell
  plays and records nothing; the permission is only for the controls. Is
  that acceptable, with a sentence in About? (The alternative is no 003.)
- **Q3:** cover art on the lock screen is an http URL on the player. Whether
  the lock screen loads remote art is not documented. If it does not, the
  controls work without art. Acceptable?

## Acceptance criteria

1. While Swell runs and a player is current, the lock screen shows Swell's
   controls with title and artist. Play/pause, next and previous act on the
   player, and next and previous are disabled when the player cannot skip.
2. The controls follow a change of current player, and changes made in the
   official app or on the player itself.
3. With no player, or with the player offline, the controls show stopped and
   do nothing.
4. The validator passes with the new import, requirement and permission. The
   RPM requires amber-qml-plugin-mpris.
5. All existing suites pass. A test checks that the MPRIS mapping (status,
   capabilities, metadata, requests) follows the player, against the fake
   player, if Amber.Mpris can be instantiated in the build engine;
   otherwise the review decides what replaces it.
6. A device check by Misja covers the lock screen, the events view, playing
   from the official app while the screen is locked, and the first-start
   permission prompt.
7. A separate result review before the PR.
