# Evidence — 006 firmware updates

## Build (2026-11-09, commit ea43753)

Branch 006-firmware, built from a clone of the branch in the SDK.

- **RPMs:** aarch64 and armv7hl build with no compiler warnings.
  - sha256 aarch64:
    b6b51f941a4a62cc6e70e455fd93b76a77936a9f4fce8a704b6afe8645e7034a
  - sha256 armv7hl:
    528bcdfeada666dc2d11eb557ee294af920330d25f509c9a6ece04ccda271b94
- **Requires** (both): sailfish-components-webview-qt5, -popups and -pickers.
- **Installed in both SDK targets with Misja's approval (2026-11-09):**
  - qtmozembed-qt5-devel 1.55.10;
  - sailfish-components-webview-qt5, -popups and -pickers 1.6.9;
  - qt5-qttest-devel 5.6.3, reinstalled.
- **Harbour validator:** passes on both, rebuilt from ea43753. Each has one WARNING: the binary's
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
| M-E: build evidence predated `-pickers` | Rebuilt and revalidated from ea43753 (above) |
| M-F: issue | #28 exists since the scope review; tasks.md updated |
| L-A, L-F, L-H | Recorded in the spec |
| L-B: `setStatusPolling(false)` keeps /Status polling, also on main | Not 006; in STATUS.md, to report separately |
| L-C: "No update found." without an answer | `upgradeChecked`; the text shows only after an answer |
| L-D: "Firmware" twice | DetailItem says "Firmware version" |
| L-E: availability as the number 4 in QML | Kept, consistent with 0, 2 and 3 |
| L-G: the tst_xml comment | Corrected |

## Device check (AC5)

Installed with Misja's approval (2026-11-09):
- Phone: SFOS 5.2.0.22, Sailfish WebView 1.7.4.1 (with -popups and -pickers).
  That is also before 1.8, so the link advice still holds there.
- Build: harbour-swell-0.5.0-2, from ea43753. Release 2 was set only in the
  SDK clone for the device. Permissions `Internet;Audio;WebView`.
- Test player: `household.py 192.168.1.18 --firmware`, run in the SDK
  container as root (port 80), approved.

Result: Misja, 2026-11-09: "werkt allemaal". This was the whole list below,
against the test player; the real NAD page opened in the web view. The test
player was stopped afterwards.

Checked:
- the notice and the Settings page;
- the web view against the NAD;
- the WebView permission prompt;
- start-up time and memory compared with 0.5.0.

A real firmware update is not verified until the NAD is offered one.
The start-up time was judged by Misja on the device, not measured.

## Test household review (2026-11-09; separate agent, accept with two fixes)

Applied in 8e0a7be:
- **M1:** silence holds a request until the restart phase ends, then drops
  it; it no longer closes at once.
- **M2:** root is dropped to the sudo user after binding port 80.
- **L1:** one update however often it is started; a second start is logged
  as REFUSED.
- **L2:** every `/upgrade` other than `upgrade=check`, POST too, is REFUSED,
  also while silent.
- **L3:** logged requests are escaped.
- **L4:** a malformed `timeout` is read as 0.
- **L5:** the docstring names the invented parts.

Checked in the SDK container on 127.0.0.1:
- the script runs as `ubuntu` after binding;
- POST `upgrade=this` gives 400 REFUSED;
- the second start is refused;
- a silent request is held for 9 s and then dropped;
- the player comes back on 9.9.2.

## After the device check (Misja, 2026-11-09)

- The notice on Home opens the player's update page directly, without the
  Settings step.
- Leaving the update page asks again (`PlayerWebPage.player`).
- `make check` passes again on all eleven suites (8e0a7be).

## A fault on main, fixed here (Misja: part of the PR, 2026-11-09)

- **The fault:** after `setStatusPolling(false)`, /Status went on being
  read. The aborted poll failed, `failed()` rescheduled it, and
  `pollStatus()` did not check `m_statusPolling`. The result review found
  it on main and on the branch alike (L-B).
- **The fix:**
  - `pollStatus()` returns while status polling is off;
  - an aborted poll's reply and an in-flight probe no longer change
    availability, which then comes from the sync watch.
- **Test:** `tst_client` `statusPollingOffStopsStatusReads`. Without the
  three guards it fails: 5 /Status reads where 2 are expected. With them it
  passes.
