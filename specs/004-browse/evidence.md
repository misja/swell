# Evidence — 004 browse

Raw build and test logs named below (`*.log`) are kept locally by the
builder, not in the repository (since 2026-11-08); this file holds the
conclusions.

Date: 2026-11-06. Branch `004-browse`. Package harbour-swell-0.3.0-12.aarch64.rpm,
sha256 993837d2b47ef6e6baf865723e3b186d0e0702ebc2c9c483570bd4c1b090c6f5.

## Automated (builder)

- `unit-aarch64.log`: QtTest in the SailfishOS-5.1.0.11-aarch64 target under
  sb2/qemu. All suites pass: spike 5, lsdp 18, xml 30, client 26, model 26,
  qml 15, queue 28, browse 26 (QtTest counts include init/cleanup).
- `sdk-build-aarch64.log`: `sfdk build`, exit 0.
- `harbour-validator.log`: passed; the known warning "file is not stripped".
- Translations: Dutch complete; English plural forms.

## Separate assistant review

- Scope/design review (review_004): approve with changes; findings applied
  to spec and plan before implementation. Pacing decided by the maintainer (option 1).
- Result review: approve with changes (B1 visible letters could stay "…";
  S1 letter index stale after inserted rows; S2 no retry of a failed page;
  S3 every page on the stack reloaded on a favourite change; S4 offline and
  untranslated errors; S5 test gaps; NITs). Fixed on the branch after
  0.3.0-1: B1, S1, S2, S3, S4, N1–N3, and tests for B1/S1/S2/N1, playPath and
  top-level search (browse 22). Mutation check: without the re-ask after a
  page, `severalVisibleLettersAllLoad` fails. Not yet built into an RPM.
  Open: N4 (covers in letter lists), N5 (unused parsed fields, kept for the
  reusable library).

## Device check (the maintainer, 0.3.0-1, 2026-11-06), first impressions

- The letter index (quick jump) works well. Letters are too close together
  for Artists and Albums: use the full screen height, as the People app does
  (the maintainer's screenshot).
- Artist page: shows "Albums" and "Tracks" links. Wish: show the albums
  directly, with Tracks as a further view.
- Tracks list: album art per track wished.
- More screenshots in ~/Downloads (recent), not yet reviewed.

Agreed on the canvas (B3, B4, B8, B9) and built into 0.3.0-2: a full-height
letter index with a star; favourites on top of Albums (cover grid) and Artists
(rows); no "recently played" (the maintainer keeps to the published API); an artist
opens on its albums with Songs in the pulley; covers per track outside an
album's own page. New tests: `anArtistOpensOnItsAlbums`,
`aLetterListFindsItsFavourites`, `noFavouritesWhenTheLibraryHasNone`.

Reviewer re-check (same reviewer, 2026-11-06): the earlier findings
resolved; on the device-feedback changes, approve with changes, no blockers.
Fixed in 0.3.0-3:
- favourites on top only for Albums and Artists (a Songs list got them too;
  new test `songListsGetNoFavourites`);
- the cover grid kept clear of the letter index;
- pressed feedback on covers;
- the favourites header reloads after a change;
- the star only when favourites exist;
- "Play all" for track lists that are not an album;
- the artist's name on the page opened from its pulley;
- no endless retries while offline;
- the observed keys recorded in the spec.
Not changed:
- no long-press menu on favourite covers (v1);
- one fixed wait in a negative test.
The reviewer read principles 1.1.0, because its local `origin/main` predated
the 1.2.0 merge (13985f2); it applied the UI-check items as listed in its
brief.

The maintainer's feedback on 0.3.0-2 (screenshot 2026-11-06 23:39), fixed in 0.3.0-4:
- The Favourites row on Browse showed "NaN:NaN" as a duration. Fixed: the
  row now has no duration, and a missing duration formats as nothing.
- The letter index lit a letter only while it was touched. It now keeps the
  letter of the part in view lit, as in People (`BrowseModel::sectionAt`, which
  does not read the row).

- The header per letter is removed in 0.3.0-5 (the maintainer: confusing and
  cluttered). The index alone shows where the list is (spec decision 7).

- TuneIn's menu images (sent by the player per item) showed at cover size
  on 0.3.0-5 (screenshot 2026-11-07 00:01). In 0.3.0-6 link rows get an
  icon-sized image on a normal row; covers stay for albums, tracks, stations.
- TuneIn has its own Favourites link (seen in that menu). Not wired into
  Swell's Favourites entry (v1: Library only).
- Navigation (the maintainer, 2026-11-07, spec decision 12, canvas N1–N5): a play
  action pops to Home; the mini-player shows on every page except Now
  Playing; the player's `#` section is left out of the letter index (the
  player sends it first, checked read-only 2026-11-07). Result review
  (separate agent, 2026-11-07): approve with changes; M1 fixed (a
  mini-player tap during a page transition is ignored, so Home's
  openNowPlaying flag cannot stay set). Re-check of the Queue/About
  follow-up: approve; H1 applied (About's tap goes on only after a
  successful pop).
- App icon (#3): 0.3.0-7 to -10 tried the maintainer's designs; the sound-bar icon
  (`icons/harbour-swell.svg`) was accepted in the launcher (2026-11-07). The
  launcher shows a changed icon only after lipstick restarts.
- Covers (decision 13): the Media player's placeholder while a cover loads,
  only where the player sends an image (0.3.0-11 left artists, genres and
  composers on it; fixed in 0.3.0-12). Loading ahead stays Silica's default
  of 8 rows. Covers are 600×600 JPEG, 17–35 KB, ~5 ms, no cache headers.
  Result review (separate agent, 2026-11-08): approve with changes;
  applied: no extra cacheBuffer, the placeholder stays until the fade ends,
  icons per row kind, Icon `highlighted`.

## Device check (the maintainer, Xperia 10 II, NAD C 379)

- 0.3.0-6: library scrolling and letter jumps, favourites on top, artist
  pages, covers per track, TuneIn icons, actions (2026-11-07: "alles werkt
  prima tot zover").
- 0.3.0-7: navigation, play returns to Home, mini-player everywhere except
  Now Playing ("navigatie werkt", 2026-11-08).
- 0.3.0-12: cover placeholders; artists, genres and composers without one
  ("werkt", 2026-11-08).

## Not checked

- The English plural on a locale without a Swell translation.
- A player set to another language than Dutch or English.
