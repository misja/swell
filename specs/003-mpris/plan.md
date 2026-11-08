# Plan — 003 MPRIS lock-screen controls

Status: draft 2026-11-08, for the scope/design review.

## Approach

- **New `qml/Mpris.qml`:** an `MprisPlayer` with bindings to `players.current`
  (spec scope 1). Created once in `harbour-swell.qml`, next to the cover.
- **`raiseRequested`:** `app.activate()`.
- **Position:**
  - `position` is a binding to `_player.position * 1000`. It is refreshed on
    `positionRequested`, which amber emits when a client reads the position.
  - The client's own `positionChanged` drives the in-app progress. It is not
    forwarded at every tick; MPRIS clients extrapolate from playbackStatus.
- **No C++ changes.** The library stays QML-free. QML is the only allowed
  route (libambermpris is not an allowed library).

## Packaging

- `harbour-swell.desktop`: `Permissions=Internet;Audio`.
- `rpm/harbour-swell.spec`: `Requires: amber-qml-plugin-mpris`.
- `BuildRequires`: the build engine needs the plugin for the QML compile
  test. It is already in the target; the spec does not need to list it,
  because the import is resolved at run time. The review should confirm
  this.

## Tests

- **`tests/qml`:** the compile test covers `Mpris.qml` (the import resolves
  in the target).
- **New runtime test,** in `tests/qmlruntime` with the fake player:
  - instantiate `Mpris.qml` (needs a session bus in the build engine; if
    none, the `MprisPlayer` cannot register and the test may only check the
    bindings);
  - check playbackStatus, can*, metaData and request handling.
  - The review decides whether a D-Bus-free check of the bindings suffices.
- **Device check (AC6).**

## Risks

- The lock screen may not show art from an http URL (Q3).
- Two Swell instances cannot run at once (the booster prevents it), so the
  fixed service name does not collide.
- When the app closes, the MPRIS service goes away with it, as expected.
