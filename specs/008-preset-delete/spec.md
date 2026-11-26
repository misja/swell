# Feature specification — 008 deleting a preset

Status: draft 2026-11-26. Scope reviewed by a separate agent (request
changes, resolved in misja/swell#55).

## Owner and linked issue

- Accountable owner: the primary assistant.
- Decision owner: the maintainer.
- Issue: misja/swell#55.
- Review: a separate agent reviewed the scope before implementation and
  reviews the result before the PR.

## Problem

Deleting a preset is one of the things that still need the official BluOS
app. Swell should delete a preset of the current player from Home.

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
  - Add, for reference, not in scope:
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
  - deleting on a group secondary.

## Decisions (the maintainer, 2026-11-26)

1. Swell may use one undocumented route, measured from the official app:
   `/SetPreset?id=N`.
2. No exception to spec 004: Swell does not read `/ui/ContextMenu`.
3. A remorse comes before the delete, although the official app has none.
4. Only the current player, from Home.

## Scope

1. **Menu.** The preset tiles on Home become Silica `GridItem`s in a
   `SilicaGridView`.
   - It has `interactive: false`; its height is `contentHeight` plus
     `__silica_menu_height`, so an open menu fits inside the page.
   - Tile size, spacing and the 4 or 6 columns stay as now.
   - A long press opens a `ContextMenu` with one item, Delete. The tiles
     are disabled while the player is offline.
   - Delete is enabled only while no remorse runs (`RemorsePopup.active`,
     RemorsePopup.qml:44) and while the list shown is the one the player
     has (`Player.presetsCurrent`). So deletes go one at a time.
   - `__silica_menu_height` is a private Silica property, used only for the
     grid's height. It is named in the PR as an own solution.
   - The grid is as wide as the page and starts its tiles at the page
     margin (`leftMargin`). A tile's menu then spans the page like any
     Silica context menu: it takes the view's width (ContextMenu.qml:96).
     GridView counts its columns on its whole width (Qt 5.6.3,
     qquickgridview.cpp:451). The margin is less than a column, so there
     are 4 or 6 columns as before. On the device the first version's menu
     ran from the margin to past it on the right (2026-11-26); hence this
     change.
2. **Remorse.** A `RemorsePopup` on the page, not on the delegate. Every
   presets change recreates the delegates, and a remorse on a delegate
   would go with it.
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
5. **List.** The presets on Home come from the existing `prid` refetch. The
   reply is read only for the outcome.

## End states

| Situation | Sent | Shown |
| --- | --- | --- |
| Remorse runs out | one `/SetPreset?id=N` | gone after the next status; the other ids unchanged |
| Remorse cancelled | nothing | unchanged |
| Another player made current during the remorse | nothing | "Nothing was deleted." |
| `prid` changed during the remorse (another controller) | nothing | "The presets have changed. Nothing was deleted."; the list refreshes |
| Player offline when the remorse runs out | nothing | "Nothing was deleted." (not online) |
| No answer, HTTP error, `<error>`, or the id still listed | one request | the failure notice; list unchanged until a `prid` change |
| Player offline when the menu would open | — | tiles disabled, as now |
| Delete chosen again while a remorse runs | — | Delete disabled |
| `/Status` reports a newer list that `/Presets` has not given yet | — | Delete disabled; if chosen just before, the change refusal |
| A group secondary is current (primary unknown) | as any player | not measured; no special handling |

Tests (`tests/presets`) cover the client side of each state. The page's
side is checked on the device:
- a cancelled remorse;
- another player made current during the remorse;
- the disabled tiles and menu item;
- the layout and the menu.

The pages are not instantiated in the tests. A timeout is covered with
an HTTP error row and a no-answer row.

## Out of scope

- Adding, editing or reordering presets.
- Deleting on players other than the current one.

## Acceptance criteria

1. A delete sends exactly one `/SetPreset?id=N`, only after the remorse
   ran out. A cancelled remorse sends nothing.
2. Each refusal in the end states sends nothing, and says so.
3. A failed reply gives the failure notice; the list stays.
4. Two deletes in quick succession send two requests, at least the
   per-path interval apart.
5. The double follows the capture: the reply carries the list, `prid` rises
   by one, and gaps stay. Unmeasured behaviour is marked invented.
6. `tst_qmlruntime` `presets` and `presetsFollowThePlayer` still pass.
   `make check` and the Harbour validator pass. Dutch is complete.
7. Device check on the NAD with a throwaway preset (the maintainer's
   approval).
