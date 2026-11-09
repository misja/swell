# Evidence — 006 firmware updates

## Build (2026-11-09)

Branch 006-firmware, built from a clone of the branch in the SDK.

- **RPMs:** aarch64 and armv7hl build with no compiler warnings.
- **Harbour validator:** passes on both. Each has one WARNING: the binary's
  rpath includes the SDK's target path to xulrunner, for example
  `/home/ubuntu/SailfishOS/mersdk/targets/SailfishOS-5.1.0.11-aarch64.default/usr/lib64/xulrunner-qt5-91.13.1`.
  - It comes from `libxul.pc`, which qt5embedwidget requires: its `Libs`
    has `-Wl,-rpath,/usr/lib64/xulrunner-qt5-91.13.1`. The SDK build then
    turns it into the target path.
  - The binary does not need libxul itself; `NEEDED` has only
    `libqt5embedwidget.so.1`, which carries its own rpath to xulrunner. The
    extra rpath points nowhere on the device.
  - Not yet resolved. It needs a decision on upstream practice (open point).

## Tests

Not run: the SDK snapshots no longer have qt5-qttest-devel. A
`build-requires reset` on 2026-11-09, run after installing the WebView
packages, removed it. Written, not yet run:
- `tests/xml`: `upgradeCheck` and `syncStatusDuringUpgrade`.
- `tests/firmware` (new, 12 tests), against `FakePlayer`, for AC1 and AC2:
  - available, not available and in progress;
  - only `/upgrade?upgrade=check` is ever sent;
  - the interval;
  - a 404, a non-answer and a timeout keep what was known;
  - no check without status polling;
  - a fixed-group secondary;
  - the update page URL;
  - a full update: silence, Stage2 with no /Status and no long poll, then
    back on the new version;
  - a stage root is not NotAPlayer;
  - a stop during a check.
- `tests/group`: `/upgrade` added to the expected paths of a lone player.
