# Feature specification — 008 presets: a page, deleting and adding

Status: draft 2026-11-26; widened scope reviewed by a separate agent
(approve with changes, resolved here).
- Deleting: scope reviewed (request changes, resolved in misja/swell#55),
  built and reviewed (approve with changes, resolved).
- Widened the same day by the maintainer: a Presets page, a limited row on
  Home, and adding. The new scope needs its own review.

## Owner and linked issue

- Accountable owner: the primary assistant.
- Decision owner: the maintainer.
- Issue: misja/swell#55.
- Review: a separate agent reviewed the scope before implementation and
  reviews the result before the PR.

## Problem

Managing presets still needs the official BluOS app. Swell should show
all presets of the current player on a page of their own, and delete and
add them there. Home keeps a short row, so that many presets do not fill
it.

## Facts

Measured on a NAD C 379, firmware 4.18.26, on 2026-11-26. The raw files
stay local; `evidence.md` has the record.

- **The published API** (v1.7, §6) says presets "must be added and deleted
  using the BluOS Controller app". It documents no route for deleting.
  `/Services` declares `/SetPreset` only for adding.
- **The official app**, captured with tcpdump on the phone (Android app
  support):
  - Delete: `GET /SetPreset?id=7`, with only the id.
  - Reply: `200`, the whole `<presets prid="…">` list without that preset.
  - Add from the official app (its own slot choice and volume; Swell uses
    the player's Browse action instead):
    `GET /SetPreset?volume=66&image=…&encoded_url=…&name=…&canShuffle=0&id=7&shuffle=0`.
- **The player:**
  - `prid` rises by one with every add and delete, in `/Presets` and in
    `/Status`. `/Status` showed the new `prid` about 40 ms after the reply.
  - A gap stays: deleting 7 with 8 present leaves 8 as 8.
  - Ids are reused: 7 was deleted at 20:41 and added again at 20:55.
- **The player's own context menu** (`/ui/ContextMenu`, `type=preset`)
  declares Delete as `/SetPreset?id=N`. Swell does not use the `/ui/`
  screens (spec 004); the menu only confirms the route.
- **Supplied observation** (the maintainer): the official app asks for no
  confirmation before a delete.
- **azzurro** (jzbz/azzurro 10cce29):
  - It has no preset delete.
  - Reordering through `/Presets/edit` deletes silently on an incomplete
    permutation (Powernode 4.16.22). Swell does not use it.
- **Silica** (sailfishsilica-qt5 1.2.154 in the 5.1.0.11 target):
  - `GridItem` finds its view through `__silica_gridview` (GridItem.qml:52).
  - When a menu opens, `GridItem` moves the items below it down by
    `__silica_menu_height` (GridItem.qml:78–126).
  - `SilicaGridView` defines `__silica_menu_height` (SilicaGridView.qml:59)
    but does not add it to its `contentHeight`.
- **Not measured:**
  - other players or firmware;
  - an `<error>` or HTTP error reply;
  - deleting an id that is already gone;
  - deleting on a group secondary;
  - a full preset list (the player's maximum), adding with an `id`
    (overwrite), and a failed add.

## Decisions (the maintainer, 2026-11-26)

1. Swell may use one undocumented route, measured from the official app:
   `/SetPreset?id=N`.
2. No exception to spec 004: Swell does not read `/ui/ContextMenu`.
3. A remorse comes before the delete, although the official app has none.
4. Only the current player.
5. The Presets page, the short row on Home and adding belong to 008.
6. Adding goes to the first free slot; Swell does not ask for a slot.
7. Moving presets (`/Presets/edit`) comes later; the per-preset volume is
   left out.
8. Standard implementations as far as possible, by the Sailfish design
   guidelines.
9. Home's row only plays: no menu there. The page is reached only from
   the pull-down menu.
10. Home shows the first 8 presets in portrait (two rows of 4) and the
    first 6 in landscape (one row), by id. The page always has all of
    them.
11. Home's pull-down menu, from the top: About, Players, Settings,
    Presets, Browse. The items for the current player are nearest the
    content, so they come first when pulling.
12. Short texts: "Add preset" and "Added to presets".

## Scope

### Pages

A. **Presets page.** A new page, `PresetsPage.qml`, with all presets of
   the current player.
   - The page's own scrolling `SilicaGridView`, with `anchors.fill:
     parent`, is a standard Silica grid:
     - `GridItem` delegates that fill their cells (GridItem.qml:47–48),
       4 or 6 columns;
     - a `PageHeader` in `header` ("Presets", with the player's name);
     - a `ViewPlaceholder` when there are none, which says presets are
       added from Browse.
   - Each tile has a context menu with Delete (below). The menu takes the
     grid's width, which is the page's (ContextMenu.qml:96).
   - The page follows the current player. Its tiles are disabled while
     the player is offline.
B. **Home.** A row that only plays: the first 8 (portrait) or 6
   (landscape) presets by id, in a page-wide, non-interactive
   `SilicaGridView`.
   - The tiles have no menu. So nothing opens inside the grid, and the
     grid needs no menu height: it is `rows * cellHeight`.
   - No "more" element: the page is in the pull-down menu, always.
   - `Player.presets` is in the player's order, which was ascending by id
     in every measured list. Home takes the lowest ids itself, so it does
     not rely on that order.
   - The text for no presets points to Browse.
C. **Shared.** The tile is one component, `PresetTile.qml` (a `GridItem`).
   It holds the cover or the input icon and the name, and has no menu.
   Each page sets the tile's `menu` itself. The remorse and the delete
   function are on the Presets page only.
D. **Adding.** In Browse, an item whose context menu from the player has
   an action of type `preset` offers "Add preset".
   - `preset` is added to `BrowseModel::knownAction`, and to `_actionText`
     in BrowsePage.
   - Swell runs the player's `actionURL` as given, through `runAction`.
     Without an `id`, the player took the first free slot in the
     measurement: a gap at 10 was filled, otherwise the next slot after
     the last (one device).
   - `actionDone("preset")` gives "Added to presets". It does not start
     playback, so Browse stays open, and nothing is reloaded. The list
     follows from the `prid` change.
   - The item is offered exactly where the player sends it. A station
     that is already a preset can be added again; the player allows it
     (measured).
   - The notice is lost if Browse is no longer the active page when the
     reply comes, as for the other actions.

### Deleting (on the Presets page)

1. **Menu.** A long press on a tile opens a `ContextMenu` with one item,
   Delete.
   - Delete is enabled only while no remorse runs (`RemorsePopup.active`,
     RemorsePopup.qml:44) and while the list shown is the one the player
     has (`Player.presetsCurrent`). So deletes go one at a time.
   - Tried and dropped on Home, on the device, 2026-11-26:
     - a grid inset by the page margin: its menu ran from the margin to
       past it on the right;
     - a page-wide grid with `leftMargin`: Qt 5.6.3's GridView sets
       `contentX` to 0 on every width change (qquickgridview.cpp:2119
       and 409), so the tiles started at the screen edge;
     - a menu on Home's row: it needed the private
       `__silica_menu_height`, and decision 9 removes it.
2. **Remorse.** A `RemorsePopup` on the page, not on the delegate. Every
   presets change recreates the delegates, and a remorse on a delegate
   would go with it.
   - When the page is left, or the popup is destroyed, while the remorse
     runs, Silica carries the action out at once (RemorsePopup.qml:186–200).
     So nothing is lost silently. A player cannot be switched without
     leaving the page.
   - When Delete is chosen, Swell records the player, the preset id and
     the version of the list shown (`Player.presetsPrid`). The status
     `prid` can be newer before `/Presets` has answered.
3. **Sending.** When the remorse runs out, Swell calls
   `Client::deletePreset(id, prid)`. It sends nothing, and reports a
   refusal, when:
   - the player is no longer current, or is gone (checked on the page);
   - the player is not online;
   - the recorded version is not the list's version now, or `/Status`
     reports a newer one (`Player.presetsCurrent` is false).
   Otherwise it sends `/SetPreset?id=N` through `fetch()`, paced by path
   like `runAction` and `/Delete`. It does not go through the merging gate.
4. **Outcome.** The reply decides the outcome. Success is a `<presets>`
   document without that id. Anything else (no answer, an HTTP error,
   `<error>`, a document that still lists the id) gives `commandFailed`,
   and with that the existing failure notice.
5. **List.** The presets on the page and on Home come from the existing
   `prid` refetch. The reply is read only for the outcome.

## End states

| Situation | Sent | Shown |
| --- | --- | --- |
| Remorse runs out | one `/SetPreset?id=N` | gone after the next status; the other ids unchanged |
| Remorse cancelled | nothing | unchanged |
| The page left while the remorse runs (back, or to Now Playing) | one `/SetPreset?id=N` at once (Silica, RemorsePopup.qml:186–200) | as when it runs out |
| `prid` changed during the remorse (another controller) | nothing | "The presets have changed. Nothing was deleted."; the list refreshes |
| Player offline when the remorse runs out | nothing | "Nothing was deleted." |
| No answer, HTTP error, `<error>`, or the id still listed | one request | the failure notice; list unchanged until a `prid` change |
| Player offline while the page is open | — | tiles disabled; the page stays |
| Another player made current while the page is open | — | the page shows that player's presets |
| Delete chosen again while a remorse runs | — | Delete disabled |
| `/Status` reports a newer list that `/Presets` has not given yet | — | Delete disabled; if chosen just before, the change refusal |
| A group secondary is current (primary unknown) | as any player | not measured; no special handling |
| Add preset | the player's `actionURL` | "Added to presets"; the tile appears after the next status |
| Add while offline, no answer, HTTP error, `<error>` | one request (offline: the request fails) | the failure notice |
| 8 or fewer presets (portrait; 6 in landscape) | — | Home shows all |
| More | — | Home shows the first 8 or 6 by id; the page has all |
| Rotation | — | Home switches between 8 and 6 |
| No presets | — | the page's placeholder; Home's text points to Browse |

Tests (`tests/presets`, `tests/browse`) cover the client and model side:
- deleting, with each refusal and failure;
- adding, with the measured `actionURL` kept by `actionsOf` and passing
  `splitPlayerPath` unchanged;
- the double's slots.

The pages are not instantiated in the tests. The device check covers:
- the layout of the page and the Home row;
- the pull-down order;
- the menu, a cancelled and a completed remorse;
- leaving the page during a remorse;
- add from Browse (Radio Paradise and TuneIn) and its notice.

## Out of scope

- Moving (reordering) presets, editing a preset, the per-preset volume.
- Choosing a slot when adding.
- Adding inputs as presets; the player sends no action for them.
- Presets of players other than the current one.

## Acceptance criteria

1. A delete sends exactly one `/SetPreset?id=N`, only after the remorse
   ran out. A cancelled remorse sends nothing.
2. Each refusal in the end states sends nothing, and says so.
3. A failed reply gives the failure notice; the list stays.
4. Two deletes in quick succession send two requests, at least the
   per-path interval apart.
5. The double follows the capture: the reply carries the list, `prid` rises
   by one, and gaps stay. Unmeasured behaviour is marked invented.
6. "Add preset" is offered exactly on browse items whose context menu
   has an action of type `preset`. It sends that `actionURL` unchanged,
   once. The double answers as measured: the first free slot, gaps filled,
   duplicates allowed.
7. Home shows at most 8 (portrait) or 6 (landscape) presets, by id, and
   has no menu. The page shows all presets and deletes as in 1–4. The
   pull-down order is as in decision 11. These are checked on the device.
8. `tst_qmlruntime` `presets` and `presetsFollowThePlayer` still pass.
   `make check` and the Harbour validator pass. Dutch is complete.
9. Device check on the NAD with throwaway presets (the maintainer's
   approval): the list under End states.
