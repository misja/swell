# Evidence — 004 browse

Date: 2026-11-06. Branch `004-browse`. Package harbour-swell-0.3.0-3.aarch64.rpm,
sha256 9ca277ea2c8bb0a0d30d740573afdbc5c1ba3bb09757b42e527b6db5864af892.

## Automated (builder)

- `unit-aarch64.log`: QtTest in the SailfishOS-5.1.0.11-aarch64 target under
  sb2/qemu. All suites pass: spike 5, lsdp 18, xml 30, client 26, model 26,
  qml 15, queue 28, browse 26 (QtTest counts include init/cleanup).
- `sdk-build-aarch64.log`: `sfdk build`, exit 0.
- `harbour-validator.log`: passed; the known warning "file is not stripped".
- Translations: Dutch complete; English plural forms.

## Separate assistant review

- Scope/design review (review_004): approve with changes; findings applied
  to spec and plan before implementation. Pacing decided by Misja (option 1).
- Result review: approve with changes (B1 visible letters could stay "…";
  S1 letter index stale after inserted rows; S2 no retry of a failed page;
  S3 every page on the stack reloaded on a favourite change; S4 offline and
  untranslated errors; S5 test gaps; NITs). Fixed on the branch after
  0.3.0-1: B1, S1, S2, S3, S4, N1–N3, and tests for B1/S1/S2/N1, playPath and
  top-level search (browse 22). Mutation check: without the re-ask after a
  page, `severalVisibleLettersAllLoad` fails. Not yet built into an RPM.
  Open: N4 (covers in letter lists), N5 (unused parsed fields, kept for the
  reusable library).

## Device check (Misja, 0.3.0-1, 2026-11-06), first impressions

- The letter index (quick jump) works well. Letters are too close together
  for Artists and Albums: use the full screen height, as the People app does
  (Misja's screenshot).
- Artist page: shows "Albums" and "Tracks" links. Wish: show the albums
  directly, with Tracks as a further view.
- Tracks list: album art per track wished.
- More screenshots in ~/Downloads (recent), not yet reviewed.

Agreed on the canvas (B3, B4, B8, B9) and built into 0.3.0-2: a full-height
letter index with a star; favourites on top of Albums (cover grid) and Artists
(rows); no "recently played" (Misja keeps to the published API); an artist
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

## Not yet done

- Device check of 0.3.0-3.
