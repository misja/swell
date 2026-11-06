# Evidence — 001 core controller

## Current result (HEAD, release 0.1.0-6)

These logs are committed for this commit:
- `sdk-build-aarch64.log`: `sfdk build`, exit 0.
- `harbour-validator.log`: succeeded; 1 accepted warning ("file is not
  stripped").
- `unit-aarch64.log`: all suites pass: spike 5, lsdp 18, xml 19, client 26,
  model 26, qml 11.

RPM sha256 cb1765cceb3b508cb7e45e3a69ea91d2f21fc82b20989cc0460df47dc0a43872.
Dutch translation: 0 unfinished.

The table below is the history. Entries for builds 0.1.0-2…5 record results
observed at that time; their logs were overwritten by later runs. Device
results come from Misja's observation on the device and are not logs.


| Date | Check | Result | Log |
|---|---|---|---|
| 2026-11-06 | `sfdk build` (SDK 3.13.5, SailfishOS-5.1.0.11-aarch64, no-pull-build-requires), scaffold only | exit 0; RPM sha256 7da2de0ace56fea7ca86f616289eae5964792fe957d263fb7d4de1e936bd4e3d | sdk-build-aarch64.log |
| 2026-11-06 | `sfdk check -s harbour` (sdk-harbour-rpmvalidator 1.110) | succeeded; 1 warning: `/usr/bin/harbour-swell` not stripped (open) | harbour-validator.log |

First validator run failed: `%license` installed to `/usr/share/licenses`, which
Harbour does not allow; LICENSE is now installed in `/usr/share/harbour-swell`.
Build tree: `/home/ubuntu/swell-001` in the `sailfish-sdk` container.
| 2026-11-06 | QtTest spike `tests/spike` via `sfdk build-shell` + `make check` (aarch64, qemu-user) | 5 passed, 0 failed; loopback TCP+HTTP via QNetworkAccessManager, UDP datagram, `qWait(200)` = 207 ms. i486 fallback not needed | spike-aarch64.log |

`qt5-qttest-devel` was already present in the build snapshot
(`SailfishOS-5.1.0.11-aarch64.default`); `zypper in` reported "Nothing to do",
so no package was installed (approval granted by Misja 2026-11-06).
| 2026-11-06 | Unit tests `tests/` (spike, lsdp, xml) via `make check`, aarch64 | spike 5/5, lsdp 18/18, xml 14/14 passed | unit-aarch64.log |
| 2026-11-06 | + client tests (`tests/client`, in-process FakePlayer), aarch64 | spike 5/5, lsdp 18/18, xml 14/14, client 14/14 passed | unit-aarch64.log |
| 2026-11-06 | + model tests (`tests/model`, injected LSDP packets), aarch64 | spike 5/5, lsdp 18/18, xml 14/14, client 14/14, model 20/20 passed; no compiler warnings | unit-aarch64.log |
| 2026-11-06 | Mutation check: rate gate disabled in a build copy | `oneSecondRule` (gap 29 ms) and `volumeBurstCollapses` failed as expected; source restored (cmp identical) | — |
| 2026-11-06 | + prid, preset and capture-input parsers | spike 5/5, lsdp 18/18, xml 19/19, client 14/14, model 20/20 passed | unit-aarch64.log |
| 2026-11-06 | + MVP client/model: encoded-query gate, presets/inputs, playPreset/playInput (byte-exact incl. %25), error bodies, current player and polling policy | spike 5/5, lsdp 18/18, xml 19/19, client 22/22, model 23/23 passed | unit-aarch64.log |
| 2026-11-06 | AC4 spike: QQmlComponent on the minimal platform (ApplicationWindow + Nemo.Configuration + Swell types; negative control) | 4/4 passed; Silica warns only about pixel ratio/DPI (no display). Added to make check; all suites pass | qml-spike-aarch64.log, unit-aarch64.log |
| 2026-11-06 | UI part 1 (Players, Add player, Home with presets/inputs, About, persistence, start state): all suites pass (qml 10/10 compiles every app QML file); sfdk build exit 0; Harbour validator succeeded, 1 warning (not stripped). RPM sha256 35044141c62f2a7819cb61145918fa2b970e58ff320336ed726864e2e6d4e825 | unit-aarch64.log, sdk-build-aarch64.log, harbour-validator.log |
| 2026-11-06 | UI part 2 (Now Playing attached to Home with landscape layout, docked mini-player, cover with 2 actions): all suites pass (qml 11/11); sfdk build exit 0; Harbour validator succeeded, 1 warning (not stripped). RPM sha256 7393c181904a7948fe4fc8c92241ec21e2a7bb3910a0a53abd1316720ac7c57b. Limitation: the compile test does not catch runtime ReferenceErrors (one, isLandscape on the window, found by reading and fixed) | unit-aarch64.log, sdk-build-aarch64.log, harbour-validator.log |

## Device check, 2026-11-06 (manual, with Misja)

Xperia 10 II (aarch64), Sailfish OS 5.2.0.20, Wi-Fi, a real BluOS player
("Woonkamer", 192.168.1.72). RPM sha256 7393c181… installed through
`sfdk deploy --manual` + `sdk-deploy-rpm` (confirmed on the device).

- LSDP discovery works under Sailjail with only the `Internet` permission: the
  app bound UDP 0.0.0.0:11430 and found the player without a manual address.
- Players, Home (6 presets, 9 inputs), Now Playing (portrait and landscape),
  the docked mini-player: rendered as designed (screenshots reviewed in the
  session; not committed because they show third-party station logos).
- Controls work (Misja): presets, inputs, play/pause, volume. Prev/next are
  disabled for a radio stream, as the API requires (`streamUrl`).
- Observed: preset artwork appears after a short delay; 8 TCP connections to
  the player while running (Qt keep-alive pool plus the QML image loader;
  not yet checked for growth over time).
- Found and fixed: Home showed 5 instead of 6 preset tiles per row in
  landscape (spacing counted for 4 columns).
- `sfdk deploy --sdk` fails on this device: `sdk-deploy-rpm` gets no D-Bus
  session bus over SSH ("Not connected to D-Bus server"). Workaround: run it
  with `DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$(id -u)/dbus/user_bus_socket`.
- Release-target note: the device runs 5.2.0.20; the workspace registry names
  5.2.0.18.
- Follow-up build 0.1.0-2 (sha256 2e2d6eff958b08c58ae8e18fee012b1fea9a89b968b49088b86b29e0232b9371):
  - it picks the only player found on first start;
  - smaller input icons;
  - all suites pass, the validator succeeded, installed on the device.
- Inputs: hiding a source in the official app ("Hide source", a player
  setting) removes it from `/RadioBrowse?service=Capture`. Verified after
  Misja changed it: 4 inputs remain. Swell follows the player; no local
  hide list.
- Build 0.1.0-3 (sha256 3b64393d165101af0a93379eca197dd89140f06c6027cbd23e9f1626e5c60200):
  - Home is attached to Players (swipe left), on one horizontal axis with
    Now Playing;
  - presets that select an input show that input's icon on a tile;
  - cover titles wrap to two lines;
  - all suites pass, the validator succeeded, installed on the device.
- Build 0.1.0-4 (sha256 c2e351adbf94c84169dcc521b7910fb6e43397d1c72dad5ac2189886b587e8c4): previous/next follow the source (queue /Skip+/Back, stream actions per API 4.8, otherwise hidden; the cover shows play/pause only); all suites pass (xml 20, client 23); validator succeeded.
- 0.1.0-4 on the device (Misja, 2026-11-06): on plain radio, Now Playing shows only play/pause and the cover play/pause only; confirmed working.
- Build 0.1.0-5 (sha256 e8c3627c28f15b072eeb72440959cc23717a27d7b6cbc3955c102701ba86e645): Now Playing centres its controls in the remaining space (portrait and landscape); Dutch translation of all 33 strings; all suites pass; validator succeeded.
