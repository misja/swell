# Plan — 003 MPRIS lock-screen controls

Status: revised 2026-11-08 after the scope/design review. Q1 and Q2
answered (spec.md).

## Approach

- **New `qml/Mpris.qml`:** an `MprisPlayer` with `property QtObject player`
  and the mapping in spec scope 1.
  - No reference to `app` or Silica, so the runtime test can load it.
  - The locked properties are literals, each with a comment saying why.
- **`harbour-swell.qml`:** creates `Mpris { player: players.current;
  onRaiseRequested: app.activate() }` next to the cover. It is never inside
  a Loader that depends on the player, and serviceName is never empty.
- **No C++ changes.** The library stays QML-free, and QML is the only allowed
  route (libambermpris is not an allowed library).

## Packaging

- `harbour-swell.desktop`: `Permissions=Internet;Audio`.
- `rpm/harbour-swell.spec`: `Requires: qml(Amber.Mpris)` and a Release bump.

## Tests

- **`tests/qml`:** the compile test covers `Mpris.qml`, which shows that the
  import resolves in the target.
- **`tests/qmlruntime`:** the assertions in spec AC5. The test runs with
  `DBUS_SESSION_BUS_ADDRESS=unix:path=/nonexistent`, set before
  `QGuiApplication`, so it never registers on a real bus. In 1.2.9 the
  object still works without one. There is no stub Amber module and no
  D-Bus round trip: the device check covers that path.
- **Device check (AC6).**
- Evidence: conclusions in `specs/003-mpris/evidence.md`; raw logs locally
  in `evidence/003`.

## Risks

- Q1: Swell owning the media controls and headset keys while BluOS plays.
- The permission prompt on upgrade (Q2). Denying it is Sailjail's launch
  decision for the whole app, as with Internet today.
- The lock-screen QML that was read is a community copy of unknown version;
  the device check is what counts.
