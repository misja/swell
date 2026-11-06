# Feature specification — 004 browse

Status: draft 2026-11-06. UI agreed with Misja on the design canvas; for the
combined scope/design review. Not approved for implementation yet.

## Owner and linked issue

- Accountable owner: root (primary assistant session).
- Decision owner: Misja Hoebe (@misja).
- Issue: https://github.com/misja/swell/issues/7.
- UI: design canvas page "Browse (004)", boards B1–B7, agreed by Misja on
  2026-11-06.
- Review: a separate agent reviews scope and design together before
  implementation, and the result before the PR.

## Problem and intended users

After 002, a user can play presets and inputs and manage the queue, but
cannot pick an album from the local library, find a radio station or search.
The official app offers this, but mixes inputs, services and hand-offs in one
"Music sources" page and shows favourites per service through a dropdown,
which Misja finds unclear.

## Decisions (Misja, 2026-11-06)

1. Entry: a pulley item "Browse" on Home.
2. One "Favourites" entry at the top of Browse.
3. Only what the documented Custom Integration API can drive (v1.7 §7.1
   `/Browse`, §7.2 search). Spotify, whose root item only hands playback to
   the Spotify app, is left out. Inputs stay on Home.
4. The undocumented `/ui/` screens, which the official app uses and azzurro
   reproduces (azzurro docs/protocol.md, "The server-driven UI", commit
   9375da6), are not used. Any exception goes to Misja as a separate decision.
5. A clean, Sailfish-native design; the official app's structure is not
   copied.
6. UI (canvas): Browse sits nearest the content in Home's pulley (About,
   Players, Browse). Rows have no chevrons, and category rows such as search
   results have no icons.
7. Letter lists work like the People app: one list with a header per letter
   and a letter index on the side.
8. The album page shows the cover beside the title. Tapping a track plays the
   album from that track.

## Facts (read-only requests to the NAD C 379, 2026-11-06)

- Top-level `/Browse` is a `menu` of `link` items (Playlists `BluOS:`,
  Library `LocalMusic:`, Radio `Airable:`, Radio Paradise, TuneIn) and `audio`
  items (the four inputs with `inputType`, and Spotify with
  `playURL="/Play?url=Spotify%3Aplay"`).
- Library (`LocalMusic:`) is a `menu`: Artists, Albums, Songs, Genres,
  Playlists, Composers and Favourites (`LocalMusic:MG/LocalMusic-Favourites`,
  itself a menu of Artists, Albums, Songs and Playlists). It has a
  `searchKey`.
- Albums is a `sections` list: one `section` item per letter, with the count
  in `text2`. A section is a list of `album` items with `browseKey`,
  `playURL`, `text`, `text2` (artist), `image` and, with
  `withContextMenuItems=1`, an inline `<contextMenu>`. The section of 73
  albums came in one reply.
- The inline context menu of an album has Favourite (`favourite-add`), Play
  now (`add-now`), Shuffle (`add-shuffle`), Add next (`addAll-next`) and Add
  last (`addAll-last`), each with an `actionURL` that is a player path.
- Radio (`Airable:`) is a list of links (popular, Netherlands, local,
  podcasts, genres, location) and has a `searchKey`.
- Playlists (`BluOS:`) is empty on this player.
- An album (`LocalMusic:MG/LocalMusic-Album?…`) is a `tracks` list whose
  `<browse>` element has a `playURL` for the whole album. Each `track` has:
  - `text` ("1. How Did This Happen?!") and `duration`;
  - `playURL` (that file only);
  - `autoplayURL`, documented as "add a track to the queue and play it and
    add subsequent tracks from the containing object (such as an album) to the
    auto-fill section";
  - an inline context menu: Favourite, Play now (`add-now`), Add next, Add
    last.
- The API documents `nextKey` for paging ("the paging chunk size is not under
  the control of the API user"), `parentKey`, `searchKey`, `playURL`,
  `autoplayURL`, `actionURL` and the context menu types (§7.1).

## Scope (proposed; the UI is settled on the canvas)

1. **Browse page**, opened from Home's pulley. It lists Favourites first, then
   the `link` items of the top-level `/Browse` (Library, Radio, Radio
   Paradise, TuneIn, Playlists). `audio` items at the top level (inputs,
   Spotify) are not shown.
2. **Favourites** opens the Library favourites menu. Other services' favourites
   are added when they are seen through the documented API.
3. **One generic list page** for every deeper level. Rows show the image,
   `text` and `text2`.
   - Tapping a `link` opens the next level.
   - Tapping a playable item plays it (`playURL`).
   - A `sections` reply (A–Z, each with its count in `text2`) becomes one
     continuous list. It has a `SectionHeader` per letter and a letter index
     on the side, like the People app. Its length is known from the counts,
     and each letter loads when it comes into view.
   - A `tracks` list (an album) has a header with the cover, title, artist and
     track count. Tapping a track uses its `autoplayURL`, or its `playURL`
     when there is none. The pulley offers "Play album" (the list's
     `playURL`) and, when the menu offers it, Shuffle.
   - Further pages load through `nextKey` as the user scrolls.
4. **Long press** shows the item's context menu, limited to the documented
   types that Swell understands: play now, shuffle, add next, add last, and
   favourite or unfavourite. A notice confirms the action.
5. **Search** through the pulley:
   - on Browse, across everything (`/Browse?q=`);
   - in a service, within it (`searchKey`).
6. **States:** loading, empty (for example "No favourites in Library"), an
   error from the player, and the player offline.

Excluded and left for later: the `/ui/` screens, Spotify, editing playlists,
recent searches, Home sections from `/Browse`, and favourites from services
that are not seen through `/Browse`.

## Required behavior

- **Keys and paths are sent as received.**
  - `browseKey`, `nextKey` and `searchKey` are sent byte for byte as the `key`
    query value, percent-encoded once.
  - `playURL` and `actionURL` are player paths. They are sent as they are, and
    only when they start with `/`; a scheme or host is refused. This follows
    the input rule from 001.
- **Requests** go through the per-path gate and the one-second rule. Browse
  reads get the longer read deadline, because covers can delay replies.
- **Errors:** an `<error>` reply is shown with its message, translated where it
  is Swell's own text.
- **Parsing:** unknown elements and types are ignored, and malformed documents
  never crash.
- **Conventions:** the Silica conventions and the workspace principles 1.1.0
  apply, and all strings are translated.

## Acceptance criteria (draft)

1. **Parser tests** for menu, sections, album/track/station lists, an inline
   context menu, `nextKey`, `searchKey`, an `<error>` reply, and malformed
   input.
2. **Fixture tests:**
   - keys sent byte for byte;
   - paging through `nextKey`;
   - search, top-level and per service;
   - context actions sent verbatim;
   - a path with a scheme or host is refused;
   - an error is reported.
3. **QML compile test** covers the new pages.
4. **RPM:** it builds and passes the Harbour validator, and Dutch is complete.
5. **Device check by Misja:** browse the library, play an album, add next and
   last, favourite, search, and radio.
6. **A separate result review** before the PR.

## Platform facts (SDK target 5.1.0.11, 2026-11-06)

- Silica offers `SectionHeader` and the `SilicaListView` quick-scroll handle
  (`quickScroll`), both allowed in Harbour apps.
- The People app's letter bar is `SymbolScrollBar.qml` in Sailfish.Contacts.
  It is not exported in that module's qmldir, and it is tied to the contacts
  display-label model. Swell therefore needs a small letter index of its own:
  a column of letters on the side that jumps with `positionViewAtIndex`. It
  follows the look of `SymbolScrollBar` (Theme sizes, highlight colour).

## Open questions

- The Favourites key: the Library menu names it
  `LocalMusic:MG/LocalMusic-Favourites`. Can Swell rely on that key, or should
  it find the item in the Library menu? (Assumption: the key does not depend
  on the language; to be checked.)
- Whether other services return favourites through `/Browse` (TuneIn when
  signed in; Radio "My stations"). Not seen yet.
- How long lists are paged in practice (all 73 albums of a section came in one
  reply).
