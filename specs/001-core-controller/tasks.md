# Feature tasks — 001 core controller

Linked issue and plan: https://github.com/misja/swell/issues/1; plan.md in this directory.
Use this file for bounded implementation steps, not a second status board.

- [x] Scaffold from the SDK `sailfishos-qtquick2app` qmake template (harbour-swell); empty app builds for aarch64.
- [x] Spike: loopback QTcpServer + QUdpSocket QtTest via `make check` in the aarch64 target; record the result (i486 fallback only as a recorded deviation).
- [x] `lsdp::parse` and `Discovery` with unit tests (AC2 cases).
- [x] `parseStatus` / `parseSyncStatus` / `parseVolume` with unit tests on synthetic fixtures.
- [x] `PlayerClient` (shared NAM, long poll, one-second rule, backoff, commands, /SyncStatus policy) and `artworkUrl()`, with in-process fixture tests (AC3).
- [x] `PlayerModel` with discovery merge, manual hosts and tests.
- [ ] MVP library: `Status.prid`, `parsePresets`, `parseCaptureInputs`, client presets/inputs, `playPreset`/`playInput`, `playFailed`, `setActive`; parser and fixture tests (AC2, AC3).
- [ ] AC4 spike: a QML engine test on the `minimal` platform for one Silica page.
- [ ] Players page, add-player dialog and Home (presets and inputs), with Nemo.Configuration persistence and the start-state rule.
- [ ] Now Playing as an attached page, with landscape layout.
- [ ] Docked mini-player, cover and About; `allowedOrientations` on every page.
- [ ] AC4 engine test over all QML files.
- [ ] Build the RPM and run `sdk-harbour-rpmvalidator` (AC5); record deviations.
- [ ] Optional, non-gating: Layer B against demo-player (swell-test container on br0; focused isolation review first).
- [ ] Complete the separate result review against the spec (AC6).
- [ ] Prepare a change for maintainer review with evidence in `evidence/001/`.
