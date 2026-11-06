# Evidence — 004 browse

Date: 2026-11-06. Branch `004-browse`. Package harbour-swell-0.3.0-1.aarch64.rpm,
sha256 f11f74b4f6e43617b56184eb2e0cfb975131ee3764d218644ee6d09fca2d84fe.

## Automated (builder)

- `unit-aarch64.log`: QtTest in the SailfishOS-5.1.0.11-aarch64 target under
  sb2/qemu. All suites pass: spike 5, lsdp 18, xml 30, client 26, model 26,
  qml 15, queue 28, browse 16 (QtTest counts include init/cleanup).
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

## Not yet done

- Process the device feedback above (a UI change: agree on the canvas first).
- Build 0.3.0-2 with the review fixes; re-check by the reviewer; device check.
