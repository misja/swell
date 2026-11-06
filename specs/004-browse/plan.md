# Implementation plan — 004 browse

Status: draft 2026-11-06, for the combined scope/design review. Not approved
for implementation.

## Linked specification

specs/004-browse/spec.md; builds on 002 (main 88939d0).

## Design

| Part | Where | Reason |
|---|---|---|
| `parseBrowse` | C++, `BluOS` xml | The pure-parser pattern from 001/002. `<browse>` attributes: `type`, `serviceName`, `serviceIcon`, `searchKey`, `nextKey`, `parentKey`, `playURL`. `<item>`: `type`, `text`, `text2`, `image`, `browseKey`, `playURL`, `autoplayURL`, `contextMenuKey`, `duration`, `inputType`, and an inline `<contextMenu>` of (`type`, `text`, `actionURL`). `<category>` groups items under a title (search results). `<error>` gives the message. |
| `BrowseModel` | C++ `QAbstractListModel`, created per page from QML through `Client` | One model per pushed page, holding the key it was opened with. It has two modes, chosen by the reply `type`. **List:** items, extended through `nextKey` when the view nears the end; one request in flight. **Sections:** the letter items with their `text2` counts give a fixed length (the 002 pattern); each letter's `browseKey` loads when a row of it is shown, and rows carry a `section` role for `SectionHeader` and the letter index. Roles: text, text2, image (resolved), kind (link/playable/section/…), loaded, section, actions (the understood context menu items). Properties: title, serviceName, searchKey, playUrl, loading, error. |
| Root filter | C++ (`BrowseModel`, root only) | Keeps the `link` items of the top-level `/Browse` and drops `audio` items (inputs, Spotify). Adds Favourites first when the Library menu offers its favourites key (open question 1 in the spec). |
| Sending player paths | C++ (`Client::sendPlayerPath(path)`) | `playURL`, `autoplayURL` and `actionURL` are sent as given, only if they start with `/` and have no scheme or host, through the gate keyed by path. The reply is checked for `<error>`. A notice is emitted for add/favourite actions. |
| Context menu | C++ filter + QML `ContextMenu` | Only the documented types Swell understands: `add-now`, `add-next`, `add-last`, `addAll-now`, `addAll-next`, `addAll-last`, `add-shuffle`, `favourite-add`, `favourite-delete`. The inline menu is used (`withContextMenuItems=1`); `contextMenuKey` is fetched only when an item has no inline menu. |
| Search | C++ (`BrowseModel` opened with `searchKey` + `q`) and QML `SearchField` | From Browse, `/Browse?q=` (top level); from a source, its `searchKey`. Results are links grouped by category. |
| Pages | QML/Silica | `BrowsePage.qml` is generic: `SilicaListView`, `PageHeader` (title, service), rows as `ListItem` (artwork when present, `text`, `text2`), no chevrons, and a `ViewPlaceholder` for empty and error. Sections mode adds `section.property`, a `SectionHeader` and `LetterIndex.qml` (a small column of letters that jumps with `positionViewAtIndex`, styled after Sailfish.Contacts' `SymbolScrollBar`). A `tracks` reply shows the album header (cover beside the title) and a pulley with Play album and Shuffle. `BrowseRootPage` is `BrowsePage` on the root key. `SearchPage.qml` holds the `SearchField` with a results list. Home's pulley gains "Browse". |

## Validation

- **xml:** `parseBrowse` against fixtures shaped like the replies recorded in
  the spec: menu, sections, albums, tracks, radio, search with categories,
  error, and malformed input.
- **Fixture tests (FakePlayer):**
  - keys sent percent-encoded once and byte for byte;
  - `nextKey` paging;
  - sections loading per letter, with a fixed length;
  - root filtering;
  - `sendPlayerPath` refusing a scheme or host and sending a path verbatim;
  - context actions filtered to known types;
  - search, top-level and per source;
  - an error reply.
- **qml:** the compile test covers the new pages.
- **Build, validator, Dutch complete;** logs in evidence/004/.
- **Device check by Misja,** spec AC5.

## Review

- Next: the combined scope/design review by a separate agent.
- Then the result review before the PR.

## Risks

- Response shapes of services not seen on this player (TuneIn, Radio
  Paradise, search categories). The parser is lenient; unknown kinds show as
  plain rows that do nothing.
- The model per page keeps memory bounded by the pages on the stack.
- Large libraries: sections loaded on demand; nothing loads the whole
  library.
