# Feature specification — 008 presets: a page, deleting and adding

Status: draft 2026-11-26; widened scope reviewed by a separate agent
(approve with changes, resolved here).
- Deleting: scope reviewed (request changes, resolved in misja/swell#55),
  built and reviewed (approve with changes, resolved).
- Widened the same day by the maintainer: a Presets page, a limited row on
  Home, and adding.

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
- **An input as a preset** (2026-11-27, with curl, a throwaway preset):
  - `/Browse` and `/RadioBrowse?service=Capture` give the inputs only a
    play URL, no context menu, so the player sends no `preset` action.
  - `/Services` declares "Add preset" as `/SetPreset` with `url`, `name`
    and an optional `image`.
  - `GET /SetPreset?name=Phono&url=<the input's URL attribute, as given>&image=/images/capture/ic_vinyl.png`
    gave 200 and the whole list, with Phono in the first free slot (9),
    and `prid` 23 → 24. The list holds the URL decoded, the same form as a
    preset of an input added with the official app (id 1).
  - It was deleted again with `/SetPreset?id=9`.
- **Reordering** (2026-11-27; evidence.md has the requests):
  - The official app (User-Agent `Bluesound/4.18.1`) sends one request
    per move: `POST /Presets/edit?prid=N`, `Content-Type:
    application/json`, body `{"ordering":[{"from":a,"to":b},…]}` holding
    exactly the cycle of the slots between the old and the new place (7
    to 2: 7→2, 2→3, …, 6→7). The reply is 200 and the whole `<presets>`
    list with `prid` one higher.
  - Each body lists the moves in ascending `to` order, `from` before
    `to`, without spaces.
  - With curl, on throwaways in slots 8 and 9: an old `prid` gives
    **412** and changes nothing (one measurement, a `prid` one behind); no `?prid` gives **400**; an incomplete
    permutation (`9→8` only) gives 200 and **deletes** the preset that was
    in 8, as azzurro measured on a Powernode (4.16.22).
  - Not in the API; `/Services` does not declare it. The player's own
    `/ui/presets` offers "Reorder Presets".
  - Across a gap (8 and 10 swapped, 9 empty): 200, the gap stays; the
    permutation is over the occupied slots.
  - Not measured: an `<error>` reply, a 412 for any other reason.
- **Sailfish reordering:** Silica has no component for it. The home
  screen's launcher and top menu reorder with their own
  `EditableGridDelegate` (lipstick-jolla-home-qt5, `main/`, "License:
  Proprietary"; it imports `Sailfish.Lipstick` and
  `Sailfish.Silica.private`, which Harbour does not allow):
  - a long press enters the edit mode and starts the drag at once
    (LauncherGrid.qml:370–374);
  - in the mode, a press held 150 ms lifts the item (scaled 1.1–1.3) and
    it follows the finger; a press that moves earlier scrolls the view
    (EditableGridDelegate.qml:68–79, 116–126, 298–302);
  - held for 150 ms over another place, the model moves (`model.move`),
    and the other items slide there in 150 ms (:217–232, 329–352);
  - near the view's edge the view scrolls (:238–246);
  - releasing ends the drag; the edit mode stays. A tap ends the mode
    (LauncherGrid.qml:346–347).
  No other app on the device reorders. Swell follows this behaviour with
  Qt Quick's own drag handling; the code is Swell's (below).
- **Silica notices:** `Notices` is a singleton in `Sailfish.Silica` 1.0,
  with `show(text)` up to `show(text, duration, anchor, hOffset, vOffset)`;
  `Notice.Short` is 3000 ms (plugins.qmltypes:1147–1225).
  - `ApplicationWindow` shows it above `max(bottomMargin, keyboard)`
    (ApplicationWindow.qml:467–510; `stack.panelSize` is the keyboard,
    l.349). Swell sets `bottomMargin` to the mini-player's visible size
    (harbour-swell.qml:80), so a notice clears the mini-player.
  - It is one line in a small font, scrolls when longer, and a tap
    dismisses it (private/NoticeItem.qml). Its default anchor is set in
    C++ and not visible in the target: checked on the device.
- **Not measured:**
  - playing a preset of an input added by Swell (it would switch the
    amplifier's input);
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
7. The per-preset volume is left out. Moving presets belongs to 008
   (2026-11-27, decision 15).
8. Standard implementations as far as possible, by the Sailfish design
   guidelines.
9. Home's presets row only plays: no menu there (Home's input rows do
   get one, decision 13). The page is reached only from
   the pull-down menu.
10. Home shows two rows of presets in portrait and one in landscape, the
    first by id. The page always has all of them. The columns follow
    Jolla's Gallery: `Math.floor(width / Theme.itemSizeHuge)`, with cells
    of `width / columns` (Sailfish.Gallery ImageGridView.qml in the
    5.1.0.11 target). So a larger screen gets more columns, not larger
    tiles. Fixed counts of 4 and 6 were dropped (the maintainer: one
    device is one resolution).
11. Home's pull-down menu, from the top: About, Players, Settings,
    Presets, Browse. The items for the current player are nearest the
    content, so they come first when pulling.
12. Short texts: "Add preset" and "Added to presets".
13. Adding an input as a preset belongs to 008 (2026-11-27). The player
    offers no action for it, so Swell builds the request itself, with the
    parameters `/Services` declares for "Add preset".
14. Notices use Silica's `Notices.show()` instead of Swell's own label
    (2026-11-27), everywhere in the app.
15. Presets can be moved on the Presets page (2026-11-27), with the
    official app's request, `/Presets/edit`. A pull-down item, "Reorder",
    starts an edit mode like the home screen's; in it, tiles are dragged
    and each drop sends one move. Delete stays in the context menu,
    outside the edit mode. The list scrolls at the edge while dragging
    (one drag, one request), and a tap ends the mode, as on the home
    screen; a "Done" in the pull-down menu was dropped (the maintainer: a
    pull-down item that changes is not intuitive).

## Scope

### Pages

A. **Presets page.** A new page, `PresetsPage.qml`, with all presets of
   the current player.
   - The page's own scrolling `SilicaGridView`, with `anchors.fill:
     parent`, is a standard Silica grid:
     - `GridItem` delegates that fill their cells (GridItem.qml:47–48),
       columns as in decision 10;
     - a `PageHeader` in `header` ("Presets", with the player's name);
     - a `ViewPlaceholder` when there are none, which says presets are
       added from Browse or from an input on Home.
   - Each tile has a context menu with Delete (below). The menu takes the
     grid's width, which is the page's (ContextMenu.qml:96).
   - The page follows the current player. Its tiles are disabled while
     the player is offline.
B. **Home.** A row that only plays: two rows (portrait) or one
   (landscape) of the first presets by id, in a page-wide, non-interactive
   `SilicaGridView`.
   - The tiles have no menu. So nothing opens inside the grid, and the
     grid needs no menu height: it is `rows * cellHeight`.
   - No "more" element: the page is in the pull-down menu, always.
   - `Player.presets` is in the player's order, which was ascending by id
     in every measured list. Home takes the lowest ids itself, so it does
     not rely on that order.
   - The text for no presets points to Browse and the inputs.
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

E. **An input as a preset.** On Home, each input's row (a `ListItem`)
   gets a context menu with "Add preset". The inputs are shown only on
   Home; Browse leaves them out (browsemodel.cpp:308).
   - `Client::addInputPreset(id)` takes the input's id. It looks the input
     up in the list it read, and sends `/SetPreset` with `url` (the `URL`
     attribute as given, already percent-encoded, checked as `playInput`
     checks it, in one shared helper), `name` (its text) and `image` (the
     player's relative path, not the resolved address QML shows), the
     last two encoded with `QUrl::toPercentEncoding`. No `id`. It goes
     through `fetch()`, not the merging gate.
   - Success is a `<presets>` document whose `prid` differs from the one
     Swell had read: every add raised it by one (measured). Swell then
     emits `actionDone("preset")`. A URL cannot decide it: duplicates are
     allowed and the list holds it decoded.
   - Anything else, an unchanged list included, gives `commandFailed`. An
     unknown id or an invalid URL sends nothing and gives `commandFailed`;
     so does a list not read, or older than the one `/Status` reports
     (`Player.presetsCurrent` false), since the reply's `prid` could not
     tell the add from that change.
   - If another controller changes the list while the request runs, the
     new `prid` counts as success. Not measured; accepted.
   - Home shows "Added to presets", only while it is the active page, as
     Browse does. The notice is lost otherwise.
   - An input that is already a preset can be added again, as a station.
F. **Notices.** `app.notify()` and its label are removed; every notice is
   `Notices.show(text, Notice.Short)`, the platform's own (AGENTS.md,
   platform mechanisms).
   - Why the own label's add notice was not seen in the device check of
     2026-11-27 is not known. With `Notices` it is shown (evidence.md).
   - What changes: the notice also clears the keyboard, is one line
     (longer texts scroll), and takes taps over its area for its 3 s.
   - The pages still react only while active; `Notices` is global.

G. **Reordering.** On the Presets page.
   - **Mode.** A `PullDownMenu` with "Reorder", enabled only while the
     mode is off, the player is online, the list is current
     (`presetsCurrent`), there are two presets or more, no remorse runs
     and no move is in flight. In the mode, a tile has no context menu.
     The mode ends with a tap on a tile or beside the tiles, or when the
     page is left or the player goes offline; a drag in progress is then
     put back and nothing is sent. The header says the mode is on and how
     it ends.
   - **Drag**, as the home screen:
     - a press held 150 ms lifts the tile's content (scaled 1.1); only
       then the drag starts, with `preventStealing`, so a quick swipe
       still flicks and the pull-down menu and the back gesture keep out;
     - the content follows the finger, moved out of the delegate while
       dragged; the view keeps positioning the delegates;
     - held 150 ms over another place, `DelegateModel` `items.move()`
       moves the tile there in the view, and the others slide (`GridView`
       `move` and `displaced` transitions, 150 ms). The model itself stays
       `Player.presets`;
     - near the top or bottom edge, the grid scrolls while the finger is
       there;
     - releasing sends the move; a release where it started sends
       nothing.
   - **During a drag:** a change of `Player.presets` resets the view; the
     drag ends there and nothing is sent. A rotation puts the tile back
     and sends nothing.
   - **Sending.** `Client::movePreset(fromId, toId, prid)` builds the
     cycle over the occupied slots, in the list's order: the moved preset
     goes to `toId`, and each occupied slot from `toId` up to the old
     place moves to the next occupied slot towards the old place. The
     moves are listed by ascending `to`, as measured. Before sending, it
     checks that the sources are distinct, the destinations are
     distinct, both are the same set, and every source is in the list.
     It sends `POST /Presets/edit?prid=N`,
     `{"ordering":[{"from":a,"to":b},…]}`, paced per path, not merged.
   - **Refusals**, nothing sent: offline ("Nothing was moved."); the list
     not current, the `prid` not the one recorded at the drag's start,
     or an id not in the list ("The presets have changed. Nothing was
     moved."); `fromId == toId` (nothing to say).
   - **Outcome.**
     - 200 with a `<presets>` list holding as many presets as before and
       the moved one at `toId`: done; the list follows from the `prid`
       change.
     - 412: the change notice, and the view put back.
     - Anything else, a list that does not match included: the failure
       notice, and the view put back.
   - **One at a time.** `Player.presetMoving` (a property with NOTIFY)
     is true while a move is in flight; then, and until the list is
     current again, no drag starts.
   - Home's row shows the first presets by id, so it follows the new
     order.
   - Own code, named in the pull request: the hold and dwell timers and
     the edge scrolling (Silica has none; the home screen's is private),
     and the cycle built in C++.

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
| Add an input | one `/SetPreset?url=…&name=…&image=…` | "Added to presets"; the tile appears after the next status |
| Add an input: no answer, HTTP error, `<error>`, not a list, an unchanged `prid` | one request | the failure notice |
| Add an input: unknown id, an invalid URL, or the list not current | nothing | the failure notice |
| Add an input, Home no longer active when the reply comes | one request | no notice; the tile appears after the next status |
| Reorder: a tile dropped at another place | one `POST /Presets/edit?prid=N` with the cycle | the new order; Home follows |
| Reorder: dropped where it started, or the mode ended mid-drag | nothing | unchanged |
| Reorder: a quick swipe in the mode | nothing | the grid flicks |
| Reorder: dragged to the edge | — | the grid scrolls; one request on release |
| Reorder: a tap on a tile or beside the tiles in the mode | nothing | the mode ends |
| Reorder: the list changed meanwhile (412) | one request | "The presets have changed. Nothing was moved."; the view put back |
| Reorder: refused, list changed or not current | nothing | the same notice |
| Reorder: refused, offline | nothing | "Nothing was moved." |
| Reorder: no answer, HTTP error, `<error>`, not a list, a list that does not match | one request | the failure notice; the view put back |
| Reorder: the list changes during a drag | nothing | the drag ends; the new list |
| Reorder: rotation during a drag | nothing | the tile put back |
| Reorder: a drag while a move is in flight or the list is not current | — | not possible |
| Reorder chosen while a remorse runs, offline, or with fewer than two presets | — | "Reorder" disabled |
| Reorder: page left or player offline during the mode | nothing | the mode ends |
| No more presets than Home's rows hold | — | Home shows all |
| More | — | Home shows the first by id; the page has all |
| Rotation | — | Home switches between two rows and one |
| No presets | — | the page's placeholder; Home's text points to Browse and the inputs |

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
- add from Browse (Radio Paradise and TuneIn) and its notice;
- add an input from Home, its notice, and playing that preset;
- the other notices in their new form, with the mini-player open and in
  landscape.

## Out of scope

- Editing a preset, the per-preset volume.
- Reordering on Home.
- Choosing a slot when adding.
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
7. Home shows at most two rows (portrait) or one (landscape) of presets, by id, and
   has no menu. The page shows all presets and deletes as in 1–4. The
   pull-down order is as in decision 11. These are checked on the device.
8. `tst_qmlruntime` `presets` and `presetsFollowThePlayer` still pass.
   `make check` and the Harbour validator pass. Dutch is complete.
9. "Add preset" on an input sends one `/SetPreset` with the input's URL as
   given, its name and relative image, encoded, and no `id`. A list with
   a new `prid` gives `actionDone("preset")`; anything else, and an
   unchanged list, gives the failure notice; an invalid URL sends nothing.
   Tests against the double, whose add follows the measurement; the
   failures of an input add are invented there.
10. Every notice is Silica's `Notices.show()`; no own notice label remains.
11. A move sends one `POST /Presets/edit?prid=N`, JSON, with exactly the
    cycle between the two places, by ascending `to`, byte for byte as
    measured; the guard and the success check as in G. Tests: forward,
    backward, adjacent and across a gap (bodies as in evidence.md); a 412
    gives the change refusal and no `commandFailed`; 400, 500, a timeout
    and a list missing a preset give `commandFailed`; each refusal sends
    nothing; a second move while one is in flight. The double moves as
    measured (412, 400, the silent delete). On the device: the mode, a
    drag with the hold and the dwell, a swipe that flicks, edge
    scrolling, a tap ending the mode, a drag cancelled by rotation, and
    Reorder disabled during a remorse.
12. Device check on the NAD with throwaway presets (the maintainer's
   approval): the list under End states.
