# Feature tasks — 004 browse

Linked issue and plan: https://github.com/misja/swell/issues/7; plan.md in this directory.

- [x] UI agreed with Misja on the design canvas (B1–B7).
- [x] Combined scope/design review by a separate agent (review_004, approve with changes); findings applied to spec and plan; pacing decided by Misja (option 1).
- [x] `parseBrowse` with tests (menu, sections, albums, tracks, search menu, inline context menu, nextKey, error, malformed).
- [x] Client: key/q encoding, browse reads without the interval, hardened `sendAction` round-trip rule, additive actions through `fetch`; tests.
- [x] `BrowseModel` (segments, bounded loading, paging within a letter, fallbacks, root filter, Favourites detection); fixture tests.
- [x] Pages: Browse (generic), LetterIndex, album header, search; Home pulley; Dutch translation.
- [x] Build, validator, evidence/004.
- [x] Navigation: play returns to Home, mini-player on Browse and Search, no `#` in the letter index (decision 12, canvas N1–N5).
- [x] Cover placeholder as in the Media player (decision 13).
- [x] Device check with Misja (library scrolling and letter jumps, actions, favourites, search, radio, navigation, covers); another player language not checked.
- [x] Separate result reviews (whole feature, navigation, covers); findings applied. PR opened.
