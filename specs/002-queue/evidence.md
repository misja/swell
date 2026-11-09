# Evidence — 002 queue

Raw build and test logs named below (`*.log`) are kept locally by the
builder, not in the repository (since 2026-11-08); this file holds the
conclusions.

Date: 2026-11-06. Branch `002-queue`. Package harbour-swell-0.2.0-4.aarch64.rpm,
sha256 9da4a081b929dc681fc7d425359fb84283ceca0bd58f705e46a488ab281cfcae.

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

## Device check (the maintainer, Xperia 10 II, NAD C 379), 0.2.0-1, 2026-11-06

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
  fails). Its check of those fixes found one more: a failed adoption
  reloaded twice. Fixed in 0.2.0-4 with one shared change rule; the test
  now counts one reload (mutation check: the old rule fails it). Reviewer
  verdict: resolved after that fix. Residual, accepted: if the first reply
  of a reload already has a newer id than the status poll, one more reload
  follows until the poll catches up.

## Device check (the maintainer), 0.2.0-4, 2026-11-06

- One remove while scrolled: only that row goes, the list keeps its place.
- Two quick removes (the second started during the first remorse): both go,
  no "queue changed" notice.
- Input playing: the line is shown, the marker is dimmed, and tapping a
  track plays the queue again.
- Player state afterwards (read-only `/Playlist`, `/Status`): 14 -> 11
  tracks, queue id 6 -> 9 (one step per delete), playback undisturbed.
  the maintainer confirmed the remaining tracks are the intended ones.

## Not yet done

- The plural with a locale that has no Swell translation (not checked).
- Maintainer review of the PR.

## Fix after merge: the queue flashed on every open (0.2.0-5, 2026-11-06)

- The maintainer's report: going from the player to the queue, the list visibly
  refreshes. Cause: leaving the page marked the model stale, so each open
  reset and re-read it. Since the queue id changes on every modification,
  rows now stay while the id is unchanged; loading starts while the page is
  swiped in.
- Automated: all suites pass, queue 28 (new
  `reopeningAnUnchangedQueueKeepsTheRows` and
  `firstReadFailedWhileOffScreenIsRetriedOnReopen`); build and validator
  succeed. Package harbour-swell-0.2.0-6.aarch64.rpm, sha256
  77c88157a36d4b19285c54f60dc973bce76f3378e421646d60d2a887d6d427fc.
- Separate review of the fix: removing the rule is safe (the queue id
  survives a client stop/start; an offline spell resets the status). One
  defect, fixed: a first read that failed while off screen was never retried,
  so reopening now also reloads while the length is unknown (mutation check:
  the old rule fails the new test). Residual, accepted: after a resume or a
  player switch, the old rows show for about one status round trip.
- Device check (the maintainer, 0.2.0-6): swiping between Now Playing and the queue
  no longer flashes; after a new album (the official app replaces the queue)
  the list loads once with a smooth transition.
