# Plan — 008 deleting a preset

## Changes

- `bluos/client.{h,cpp}`:
  - `Q_PROPERTY(QString prid …)`: the status `prid`, NOTIFY `statusChanged`.
  - `Q_INVOKABLE deletePreset(int id, const QString &prid)`:
    - refuses with `presetDeleteRefused(bool presetsChanged)` when the
      player is not online or its `prid` differs;
    - otherwise one `fetch("/SetPreset", "id=N")`. A reply that is a
      `<presets>` document without the id is success; anything else is
      `commandFailed("/SetPreset", …)`.
- `qml/pages/HomePage.qml`:
  - the preset `Flow` becomes a `SilicaGridView` of `GridItem`s, with the
    same cells and a `ContextMenu` with Delete;
  - a page `RemorsePopup` records the player, the id and the `prid`, and
    checks that the player is still current before the call.
- `translations/`: the new strings, in Dutch too.

## Tests

- `tests/client/fakeplayer.h`: no change. A `handle("/SetPreset", …)` in
  the test plays the player, as in the capture (list without the id, `prid`
  + 1, gaps kept).
- New suite `tests/presets`: success, refusal on `prid`, refusal offline,
  the failure replies (HTTP error, `<error>`, id still listed; invented,
  marked as such), and two deletes in quick succession.
- `tests/qmlruntime`: the grid's presets read as before (`presets`,
  `presetsFollowThePlayer`); the page's checks before the call (player
  switched).

## Validation

`make check` in the aarch64 target, the Harbour validator on aarch64 and
armv7hl, a separate result review, then a device check on the NAD with a
throwaway preset.
