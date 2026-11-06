# Evidence — 002 queue

Date: 2026-11-06. Branch `002-queue`. Package harbour-swell-0.2.0-1.aarch64.rpm,
sha256 688c04e48a89446dea1ca01b3763a33d426d85a4f712836fe618c9cdb0c5af59.

## Automated (builder)

- `unit-aarch64.log`: QtTest in the SailfishOS-5.1.0.11-aarch64 target under
  sb2/qemu. All suites pass: spike 5, lsdp 18, xml 23, client 26, model 26,
  qml 12, queue 12 (QtTest counts include init/cleanup).
- `sdk-build-aarch64.log`: `sfdk build`, exit 0.
- `harbour-validator.log`: `sfdk check -s harbour`, passed; the known warning
  "file is not stripped" remains.
- Translations: Dutch complete (0 unfinished), including the plural
  "%n nummer/nummers". `harbour-swell-en.ts` exists only for the English
  plural forms; its other entries stay unfinished and fall back to the source.

## Not yet done

- Device check by Misja (spec AC5), including which `/Play` form works.
- Separate result review (spec AC6).
