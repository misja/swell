# Evidence — 006 firmware updates

## Build (2026-11-09, commit c7ad3c5)

Branch 006-firmware, built from a clone of the branch in the SDK.

- **RPMs:** aarch64 and armv7hl build with no compiler warnings.
  - sha256 aarch64:
    34527197566949eaa742370e394411d292a233e9457cd5bdee44e0074f14909b
  - sha256 armv7hl:
    4e5cf328410b8cd3ef7ce736dc12c18402605e5d53a3c4a5104567794ed4e6f0
- **Requires** (both): sailfish-components-webview-qt5, -popups and -pickers.
- **Installed in both SDK targets with Misja's approval (2026-11-09):**
  - qtmozembed-qt5-devel 1.55.10;
  - sailfish-components-webview-qt5, -popups and -pickers 1.6.9;
  - qt5-qttest-devel 5.6.3, reinstalled.
- **Harbour validator:** passes on both, rebuilt from c7ad3c5. Each has one WARNING: the binary's
  rpath includes the SDK's target path to xulrunner, for example
  `/home/ubuntu/SailfishOS/mersdk/targets/SailfishOS-5.1.0.11-aarch64.default/usr/lib64/xulrunner-qt5-91.13.1`.
  - It comes from `libxul.pc`, which qt5embedwidget requires: its `Libs`
    has `-Wl,-rpath,/usr/lib64/xulrunner-qt5-91.13.1`. The SDK build then
    turns it into the target path.
  - The binary does not need libxul itself; `NEEDED` has only
    `libqt5embedwidget.so.1`, which carries its own rpath to xulrunner. The
    extra rpath points nowhere on the device.
  - **Upstream practice** (researched 2026-11-09, at Misja's request):
    - The validator (sailfishos/sdk-harbour-rpmvalidator `rpmvalidation.sh`,
      master 7dd7dd5, l. 1221–1244) allows only `/usr/share/<name>/lib` and
      `$ORIGIN/../share/<name>/lib`. Any other rpath is a WARNING. It is an
      ERROR only when the RPM ships its own shared libraries, which Swell
      does not.
    - Harbour apps with a C++ binary and the WebView all link it the same
      way, with no rpath handling: harbour-piepmatz, harbour-hafenschau,
      harbour-present, harbour-webviewtest, harbour-sailslack and
      harbour-quartermaster (GitHub code search for `qt5embedwidget`). That
      they get the same warning is inferred, not seen.
    - The linking advice is for WebView 1.6.9, the version in the 5.1 SDK
      target. Upstream changed it on 2026-09-30 (sailfish-components-webview
      e4e86f1b3e, WebView 1.8.0, Gecko ESR153): apps that use the WebView
      only from QML, as Swell does, then need no link, and the warning would
      go away. 1.8 is not in the target Swell builds for.
  - **Conclusion:** keep the documented link for 1.6.9 and accept the
    warning. Drop the link once Swell targets WebView 1.8 or newer. No own
    workaround.

## Tests (2026-11-09, aarch64, commit c7ad3c5)

- **`make check`:** all eleven suites pass: spike 5, lsdp 18, xml 36,
  client 29, model 26, qml 19, qmlruntime 18, queue 28, browse 26,
  group 31, firmware 17. No compiler warnings.
- **qt5-qttest-devel:** reinstalled in both SDK targets with Misja's
  approval. A `build-requires reset` had removed it.
- **The QML compile test found a missing dependency:** WebView.qml (1.6.9)
  imports `Sailfish.WebView.Pickers`, but `sailfish-components-webview-qt5`
  does not require `-pickers`. Swell now requires it, as Harbour allows
  (`allowed_requires.conf`).

The new tests:
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

## Result review (2026-11-09; separate agent, approve with changes)

| Finding | Resolution |
| --- | --- |
| H-A: `inProgress` flipped Online and Updating while downloading | Stays Updating until a check no longer says so; test `stillAPlayerWhileDownloading` |
| M-A: the /SyncStatus watch long-polled while updating | Aborted on entering, no `timeout`/`etag` while Updating; test `noLongPollOnTheWatchWhileUpdating` |
| M-B: every polled player was checked | `setUpgradeChecks`, current player only; tests `checksCanBeSwitchedOff`, group `onlyTheCurrentPlayerIsAskedForFirmware` |
| M-C: libxul is mapped at every start | Spec corrected; start-up and memory go into the device check |
| M-D: docs out of line with the code | Spec, plan, tasks and STATUS corrected; runtime QML test `firmwareValues` added |
| M-E: build evidence predated `-pickers` | Rebuilt and revalidated from c7ad3c5 (above) |
| M-F: issue | #28 exists since the scope review; tasks.md updated |
| L-A, L-F, L-H | Recorded in the spec |
| L-B: `setStatusPolling(false)` keeps /Status polling, also on main | Not 006; in STATUS.md, to report separately |
| L-C: "No update found." without an answer | `upgradeChecked`; the text shows only after an answer |
| L-D: "Firmware" twice | DetailItem says "Firmware version" |
| L-E: availability as the number 4 in QML | Kept, consistent with 0, 2 and 3 |
| L-G: the tst_xml comment | Corrected |

## Device check (AC5)

Not done yet. It needs Misja's approval to install, and covers:
- the notice and the Settings page;
- the web view against the NAD;
- the WebView permission prompt;
- start-up time and memory compared with 0.5.0.

A real firmware update is not verified until the NAD is offered one.
