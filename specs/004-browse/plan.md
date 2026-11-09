# Implementation plan — 004 browse

Status: revised 2026-11-06 after the combined scope/design review (review_004,
approve with changes; findings applied below).

## Linked specification

specs/004-browse/spec.md; builds on 002 (main 88939d0).

## Design

| Part | Where | Reason |
|---|---|---|
| `parseBrowse` | C++, `BluOS` xml | The pure-parser pattern from 001/002. `<browse>` attributes: `type`, `serviceName`, `serviceIcon`, `searchKey`, `nextKey`, `parentKey`, `playURL`. `<item>`: `type`, `text`, `text2`, `image`, `browseKey`, `playURL`, `autoplayURL`, `contextMenuKey`, `duration`, `inputType`, and an inline `<contextMenu>` of (`type`, `text`, `actionURL`). `<category>` groups items under a title (search results). `<error>` gives the message. |
| `BrowseModel` | C++ `QAbstractListModel`, created per page from QML through `Client` | One model per pushed page, holding the key it was opened with. It is one model built from a list of **segments**, so there are no separate code paths. A plain list is one segment of unknown length, extended through `nextKey`. A `sections` reply gives one segment per letter, with its length from the letter's `text2` count, so the total is known and the letter index can jump. Rules:<br>- Loading follows the QueueModel pattern: one request in flight, plus only the latest wish. The letter in view loads once scrolling settles, and letters passed during a jump are not loaded.<br>- A letter that pages (Songs: 50 per reply, then `nextKey`) continues within its segment.<br>- If a letter brings a different number of items than its count, the reply is trusted and the rows are adjusted.<br>- If a count cannot be parsed, the letters are shown as plain link rows.<br>- The `section` role comes from the segment (the letter's `text`), never from item titles.<br>- Unloaded and loaded rows have the same height.<br>Roles: text, text2, image (resolved), kind (link/playable/section/…), loaded, section, actions. Properties: title, serviceName, searchKey, playUrl, loading, error. |
| Root filter | C++ (`BrowseModel`, root only) | Keeps the `link` items of the top-level `/Browse` and drops `audio` items (inputs, Spotify). Favourites: the root page reads `LocalMusic:` once and shows Favourites first only if an item there has `browseKey` `LocalMusic:MG/LocalMusic-Favourites`. The hard-coded key is never sent blind, and the label (translated) is never matched. |
| Sending player paths | C++ (`Client`, extending the existing `sendAction`) | `playURL`, `autoplayURL` and `actionURL` come from the player's own replies. They are sent only if they start with `/` (not `//`), and only if the path and query survive Qt's URL handling unchanged (the `playInput` round-trip rule). The host and port are always the player's own (`url()`).<br>- **Play/replace** (`playURL`, `autoplayURL`) goes through the merging gate: the last tap wins, as with presets and inputs.<br>- **Additive actions** (`add*`, `addAll*`, `favourite*`) go through `fetch`: interval only, in order, never merged (as `/Delete`), and the reply is checked for `<error>` with the shared `errorIn`. A notice confirms the action. |
| Keys and search text | C++ | `browseKey`, `nextKey`, `searchKey` and `q` are encoded with `QUrl::toPercentEncoding` (so `:/?&=+#` and an existing `%` are encoded too), matching the §7.1 examples. "C++" must stay "C++". |
| Context menu | C++ filter + QML `ContextMenu` | Only the types Swell understands, documented in §7.1 or observed (`add-shuffle`): `add-now`, `add-next`, `add-last`, `addAll-now`, `addAll-next`, `addAll-last`, `add-shuffle`, `favourite-add`, `favourite-delete`. Only the inline menu (`withContextMenuItems=1`) is used; an item without one gets no menu (no `contextMenuKey` fetch in v1). After a favourite add or delete the page reloads, so its menu label is current. |
| Search | C++ (`BrowseModel` opened with a key + `q`) and QML `SearchField` | Sent on Enter. From Browse, `/Browse?q=` without a key (top-level search, §7.2); the reply is a menu of one link per source (captured 2026-11-06). From a deeper page, the `searchKey` of the nearest page that has one. Results open in the same Browse page. `<category>` is parsed; paging inside a category is left out of v1. |
| Pacing and deadline | C++ (`Client`) | Browse reads bypass the one-second interval (the maintainer, 2026-11-06: the API sets it for long-polled status); each model has at most one request in flight. Reads reuse the existing read deadline (`queueReadTimeoutMs`, renamed to a shared read deadline). Reply handlers are guarded with `QPointer`, because a page can be popped mid-request. |
| Library API shape | C++ | New 004 code exposes typed C++ (`BrowseModel*`, structs); QML-specific conversion stays in `src/`. Cleaning up the existing `QVariantList`/`QObject*` properties is a separate task after 004. |
| Pages | QML/Silica | `BrowsePage.qml` is generic: `SilicaListView`, `PageHeader` (title, service), rows as `ListItem` (artwork when present, `text`, `text2`), no chevrons, and a `ViewPlaceholder` for empty and error. Sections mode adds `section.property`, a `SectionHeader` and `LetterIndex.qml`: a column of the letters present that jumps with `positionViewAtIndex` and keeps the section header in view after the jump. It is styled after Sailfish.Contacts' `SymbolScrollBar`, and fits in landscape by showing fewer letters, as `displayableSymbolCount` does. `quickScroll` is off while the index is shown, as in the People app (ContactBrowser.qml). A `tracks` reply shows the album header (cover beside the title) and a pulley with Play album (the list's `playURL`). Shuffle is shown only when the album row that opened the page had an `add-shuffle` action, which is passed along. `BrowseRootPage` is `BrowsePage` on the root key. `SearchPage.qml` holds the `SearchField` with a results list. Home's pulley gains "Browse". |

## Validation

- **xml:** `parseBrowse` against fixtures shaped like the replies recorded in
  the spec: menu, sections, albums, tracks, radio, search with categories,
  error, and malformed input.
- **Fixture tests (FakePlayer):**
  - two quick additive actions for different albums both reach the player
    (no merging);
  - action paths with a raw space, non-ASCII text, `#` and `%2F`: sent
    unchanged or refused;
  - keys and `q` containing `:/?&=+#`, `%20` (sent as `%2520`) and non-ASCII;
  - sections: the length comes from the counts, the letter in view loads first,
    stale wishes are dropped, paging continues within a letter, a count
    mismatch is adjusted, and an unparsable count falls back to letter rows;
  - Favourites shown only when the Library menu has its key;
  - keys sent percent-encoded once and byte for byte;
  - `nextKey` paging;
  - sections loading per letter, with a fixed length;
  - root filtering;
  - `sendPlayerPath` refusing a scheme or host and sending a path verbatim;
  - context actions filtered to known types;
  - search, top-level and per source;
  - an error reply.
- **qml:** the compile test covers the new pages.
- **Build, validator, Dutch complete;** results in specs/004-browse/evidence.md.
- **Device check by the maintainer,** spec AC5.

## Review

- Done: the combined scope/design review (review_004). One correction to it:
  Sailfish.Contacts is on the Harbour allowed-imports list (since 4.5, without
  a compatibility promise). `SymbolScrollBar` is still not exported, so the
  own letter index stands.
- Then the result review before the PR.

## Risks

- Response shapes of services not seen on this player (TuneIn, Radio
  Paradise, search categories). The parser is lenient; unknown kinds show as
  plain rows that do nothing.
- The model per page keeps memory bounded by the pages on the stack.
- Large libraries: sections loaded on demand; nothing loads the whole
  library.
