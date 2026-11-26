# Plan — 008 presets: a page, deleting and adding

## Changes

- `bluos/client.{h,cpp}`:
  - `presetsPrid` and `presetsCurrent`, NOTIFY `presetsVersionChanged`;
  - `deletePreset(id, prid)` and `presetDeleteRefused(bool)`.
- `bluos/browsemodel.cpp`: `preset` in `knownAction`.
- `qml/pages/PresetTile.qml`: the shared tile, a `GridItem` with no menu.
- `qml/pages/PresetsPage.qml`:
  - a scrolling `SilicaGridView` of tiles, each with Delete;
  - a page `RemorsePopup` that records the player, the id and
    `presetsPrid`;
  - a `ViewPlaceholder`.
- `qml/pages/HomePage.qml`:
  - a play-only row: the first 8 or 6 presets by id, in a page-wide,
    non-interactive grid;
  - the pull-down menu, from the top: About, Players, Settings, Presets,
    Browse;
  - the empty text points to Browse.
- `qml/pages/BrowsePage.qml`: "Add preset" (`_actionText`) and "Added to
  presets" (`onActionDone`).
- `qml/harbour-swell.qml`: the Presets page keeps the mini-player and
  goes back to Home for Now Playing, like Browse.
- `translations/`: the new strings, Dutch complete.

## Tests

- `tests/presets`, against a double that follows the capture and the add
  measurement:
  - delete: success with a gap kept, the refusals (a changed list, a list
    not read yet, offline), the failures (HTTP error, no answer,
    `<error>`, the id still listed; invented and marked so), two deletes
    by send time;
  - add through `runAction`: the first free slot, then a duplicate.
- `tests/browse`: a station as the NAD sent it offers `preset`. Its
  `actionURL` passes `splitPlayerPath` unchanged (`%21`, `%2520`).
- `tests/qml` compiles the new pages. `tests/qmlruntime` is unchanged.
- The pages themselves are checked on the device (spec, End states).

## Validation

- `make check` in the aarch64 target.
- The Harbour validator on aarch64 and armv7hl.
- A separate result review.
- The device check on the NAD, with throwaway presets.
