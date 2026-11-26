# Plan — 008 deleting a preset

## Changes

- `bluos/client.{h,cpp}`:
  - `presetsPrid` (the version of the list as read) and `presetsCurrent`
    (`/Status` reports no newer one), NOTIFY `presetsVersionChanged`.
  - `Q_INVOKABLE deletePreset(int id, const QString &prid)`:
    - refuses with `presetDeleteRefused(bool presetsChanged)` when the
      player is not online, or the version differs or is not current;
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
- New suite `tests/presets`:
  - success, with a gap kept;
  - a refusal on a changed list, and on a list not read yet;
  - a refusal when offline;
  - the failure replies: HTTP error, no answer, `<error>`, id still
    listed. These are invented and marked as such;
  - two deletes in quick succession, by send time (`SendLog`, now shared).
- `tests/qmlruntime` is unchanged: `presets` and `presetsFollowThePlayer`
  still pass. The page's checks (a cancelled remorse, a player switched,
  the disabled items) are left to the device check.

## Validation

`make check` in the aarch64 target, the Harbour validator on aarch64 and
armv7hl, a separate result review, then a device check on the NAD with a
throwaway preset.
