# Feature specification — 004 browse

Status: revised 2026-11-06 after the combined scope/design review (review_004,
approve with changes; findings applied here and in plan.md). Implementation
may start.

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
7. Letter lists work like the People app: one list with a letter index on the
   side. There is no header per letter (Misja, 2026-11-06: it clutters the
   list); the index lights the letter of the part in view. The player's `#`
   section (titles starting with a digit or sign) is left out of the index
   (Misja, 2026-11-07); those rows stay at the top of the list.
8. The album page shows the cover beside the title. Tapping a track plays the
   album from that track (`autoplayURL`). The API calls the choice between
   `playURL` and `autoplayURL` a user preference; Misja chose this behaviour.
9. Device feedback on 0.3.0-1 (Misja, 2026-11-06; canvas B3, B4, B8, B9):
    - **Letter index:** it runs over the full height, as in the People app, with
      a star above A that jumps to the favourites at the top of the list.
    - **Favourites on top:** favourite albums show as a cover grid at the top of
      Albums; favourite artists as rows at the top of Artists. They come from
      the Library favourites (`category=FAVOURITES`), only when the Library
      menu offers them.
    - **No "recently played":** the documented API has none, and Misja keeps to
      the published API. The undocumented `/ui/` screens are not used.
    - **Artist page:** an artist opens straight on its albums (the first link of
      the artist's menu). The other links (Songs) are in the pulley. Tapping an
      album opens it, as everywhere; Play now is in the long-press menu.
    - **Covers per track:** song lists outside an album (an artist's songs,
      Library songs, search results) show each track's cover. An album's own
      page keeps track numbers and the cover in its header.
11. No extra state in Swell (Misja, 2026-11-06): favourites and everything
    else browsed live on the player. Swell stores on the phone only what it
    already did (manual players, the current player).
10. Request pacing: the one-second rule stays where the API sets it, on status
   queries with long polling (§2: "must not make two consecutive requests for
   the same resource less than one second apart"). Browse reads are not
   interval-limited; each page has at most one request in flight. Commands
   (volume, play, …) keep their current gate; changing that is outside 004.
12. The way back (Misja, 2026-11-07; canvas N1–N5). Going in stays as it is;
    the return gets two shortcuts made from existing parts:
    - **Play returns to Home:** Play album, Play now, Shuffle, and tapping a
      track or station start playback and pop the stack to Home in one step
      (`pageStack.pop(home)`). Add next, Add last and favourites stay on the
      page with their notice. A failed command is reported on Home, as now.
    - **Mini-player on Browse and Search:** the docked mini-player, already on
      Home and Players, also shows on every Browse and Search page; a tap pops
      to Home and goes on to Now Playing. It stays open between Browse pages.
    - Library stays one entry in Browse (not flattened).

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
- Observed keys Swell relies on (not documented; NAD C 379, BluOS firmware
  as on 2026-11-06):
  - the Library letter lists are `LocalMusic:bySection:<Kind>/…`, with Kind
    Album, Artist, Song or Composer;
  - the Library Favourites menu (`LocalMusic:MG/LocalMusic-Favourites`) links
    to `LocalMusic:<Kind>/…category=FAVOURITES…`;
  - an artist (`LocalMusic:MG/LocalMusic-Artist?artist=…`) is a menu of two
    links, Albums and Songs.
  Each is only followed when the player offers it in a reply.
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
5. **Search** through the pulley, sent on Enter (not per keystroke):
   - on Browse, across everything (`/Browse?q=` without a key). The player
     answers with a menu of one link per source (`LocalMusic:Search/<text>`,
     `Airable:Search/…`, `TuneIn:Search/…`). Tapping a source shows its
     categories (Library: Artists, Albums, Songs, Composers). This replaces
     the grouped single list of board B6;
   - in a source, within it (its `searchKey`).
   Results use the same Browse page.
6. **States:** loading, empty (for example "No favourites in Library"), an
   error from the player, and the player offline.

Excluded and left for later: the `/ui/` screens, Spotify, editing playlists,
recent searches, Home sections from `/Browse`, and favourites from services
that are not seen through `/Browse`.

## Required behavior

- **Keys and paths are sent as received.**
  - `browseKey`, `nextKey`, `searchKey` and the search text `q` are encoded
    once, with every reserved character encoded (`QUrl::toPercentEncoding`).
  - `playURL`, `autoplayURL` and `actionURL` are player paths. They are sent
    as they are, only when they start with `/`, and only if they survive Qt's
    URL handling unchanged. The host is always the player's own.
- **Requests:**
  - Playing (`playURL`, `autoplayURL`) goes through the per-path gate, where
    the last tap wins.
  - Reads and additive actions (add, add all, favourite) are never merged.
    They keep only the one-second interval, so two quick "Add last" taps for
    different albums both reach the player.
  - Browse reads get the longer read deadline, because covers can delay
    replies.
- **Counts:** the leading integer of a letter's `text2`; without one the count
  is unknown and the letters show as plain rows.
- **Loading** never reads a whole library. A letter list knows its length from
  the per-letter counts, and loads the letter in view, page by page where the
  player pages it (`nextKey`).
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
   - keys and search text with `:/?&=+#`, `%20` and non-ASCII, encoded once;
   - two quick additive actions both reach the player;
   - letter lists: the length comes from the counts, the letter in view loads
     first, earlier wishes are dropped, paging continues within a letter, a
     count mismatch is adjusted, and an unparsable count falls back to letter
     rows;
   - Favourites is shown only when the Library menu has its key;
   - empty and offline states;
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

## Library size (read-only, NAD C 379, 2026-11-06)

- **Albums:** 1278 in 27 letters; the largest letter (T, 178 albums) comes
  whole in one reply, 257 KB with context menus, in 0.09 s.
- **Artists:** 688; the largest letter (S, 75 artists) is 59 KB.
- **Songs:** 15724; a letter is paged at 50 per reply with a `nextKey` (T: 1912
  songs).
- **The letter index** (the counts) is 4.6 KB, in 0.01 s.
- Misja's device check watches scrolling and letter jumps in Albums and Songs.

## Open questions

- The Favourites key: Swell shows Favourites only when the Library menu has an
  item with `browseKey` `LocalMusic:MG/LocalMusic-Favourites`. Misja's device
  check includes another player language, to confirm that the key does not
  depend on the language.
- Whether other services return favourites through `/Browse` (TuneIn when
  signed in; Radio "My stations"). Not seen yet.
- How long lists are paged in practice (all 73 albums of a section came in one
  reply).
