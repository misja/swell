# Evidence — 002 queue

Date: 2026-11-06. Branch `002-queue`. Package harbour-swell-0.2.0-1.aarch64.rpm,
sha256 1748d8ccac2ee0e3466dc0f5a6ce2eb88f62d31acc5ec49bc198cc33bf515897.

## Automated (builder)

- `unit-aarch64.log`: QtTest in the SailfishOS-5.1.0.11-aarch64 target under
  sb2/qemu. All suites pass: spike 5, lsdp 18, xml 23, client 26, model 26,
  qml 12, queue 18 (QtTest counts include init/cleanup).
- `sdk-build-aarch64.log`: `sfdk build`, exit 0.
- `harbour-validator.log`: `sfdk check -s harbour`, passed; the known warning
  "file is not stripped" remains.
- Translations: Dutch complete (0 unfinished), including the plural
  "%n nummer/nummers". `harbour-swell-en.ts` exists only for the English
  plural forms; its other entries stay unfinished and fall back to the source.

- Mutation check: accepting any verify read before `/Delete` makes
  `staleRemoveIsRefused` and `removalByAnotherControllerIsRefused` fail;
  restored, all 18 pass.

## Separate assistant review

- Result review (2026-11-06): approve with changes; 1 blocker (a remove went
  to the first row with the same identity, so a duplicate track could delete
  the playing entry) and 4 SHOULD items. All resolved: removals keep the
  chosen position and are confirmed by a one-row read before `/Delete`; a
  successful delete drops only that row; an inactive model is stale; the old
  player's queue is switched off on a player change; the missing AC2 tests
  were added; deferred window requests; retry interval in `Timing`; a
  separate "queue changed" notice. Follow-up check of the fixes: pending.
- Not verified, for the device check: whether `libsailfishapp` falls back to
  the English plural catalog for locales without a translation.

## Not yet done

- Device check by Misja (spec AC5), including which `/Play` form works.
