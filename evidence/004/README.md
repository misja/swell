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
- Result review: pending.

## Not yet done

- Device check by Misja (spec AC5).
