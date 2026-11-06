# Evidence — 002 queue

Date: 2026-11-06. Branch `002-queue`. Package harbour-swell-0.2.0-3.aarch64.rpm,
sha256 efe4c53e29cc16201808f7347ad7bae23a93448f485bcd44b54e209ab7f325f0.

## Automated (builder)

- `unit-aarch64.log`: QtTest in the SailfishOS-5.1.0.11-aarch64 target under
  sb2/qemu. All suites pass: spike 5, lsdp 18, xml 23, client 26, model 26,
  qml 12, queue 26 (QtTest counts include init/cleanup).
- `sdk-build-aarch64.log`: `sfdk build`, exit 0.
- `harbour-validator.log`: `sfdk check -s harbour`, passed; the known warning
  "file is not stripped" remains.
- Translations: Dutch complete (0 unfinished), including the plural
  "%n nummer/nummers". `harbour-swell-en.ts` exists only for the English
  plural forms; its other entries stay unfinished and fall back to the source.

- Mutation check: accepting any verify read before `/Delete` makes
  `staleRemoveIsRefused` and `removalByAnotherControllerIsRefused` fail;
  restored, all pass.

## Separate assistant review

- Result review (2026-11-06): approve with changes; 1 blocker (a remove went
  to the first row with the same identity, so a duplicate track could delete
  the playing entry) and 4 SHOULD items. All resolved: removals keep the
  chosen position and are confirmed by a one-row read before `/Delete`; a
  successful delete drops only that row; an inactive model is stale; the old
  player's queue is switched off on a player change; the missing AC2 tests
  were added; deferred window requests; retry interval in `Timing`; a
  separate "queue changed" notice. Follow-up check (same reviewer): B1, S2, S4 resolved, S3
  mostly; S1 was still open in the UI path (a later remorse sent its old
  position and was refused). Fixed: the page sends the row's current index
  (falling back to the recorded position), with a test for that sequence; a
  window dropped by a local removal is asked for again; a local row is not
  dropped if the list was read again meanwhile; whether `/Delete` changes the
  queue id is an open point in the spec.
- Not verified, for the device check: whether `libsailfishapp` falls back to
  the English plural catalog for locales without a translation.

- Mutation check: taking the read-back id without checking length and entry
  makes `foreignChangeAfterOwnDeleteReloads` fail. The queue suite passed 3
  runs in a row.

## Device check (Misja, Xperia 10 II, NAD C 379), 0.2.0-1, 2026-11-06

- Rows, play from an entry (`/Play?id=`), the input line and clear work.
- A delete changed the queue id 3 -> 4 (read from `/Playlist` and `/Status`
  during the delete), as API §2.1/§5.1 say. 0.2.0-1 then read the whole list
  again. Fixed in 0.2.0-2 after a separate design review: the new id is read
  back and adopted when length and entry match. `reload()` now takes the id
  from the first reply, because the status poll may lag (found by a new test).

- Review of the implemented adoption (same reviewer as the design): changes
  needed, both fixed in 0.2.0-3 with tests: the current marker now uses only
  a `song` from the queue id the rows have, re-applied after adoption; a
  status reply during the first read of a reload no longer restarts it
  (mutation check: without the rule, `noRepeatedReloadDuringTheFirstRead`
  fails).

## Not yet done

- Device check of 0.2.0-3: one remove keeps the list in place; two quick
  removes both go through.
