# Feature tasks — 001 core controller

Linked issue and plan: https://github.com/misja/swell/issues/1; plan.md in this directory.
Use this file for bounded implementation steps, not a second status board.

- [x] Scaffold from the SDK `sailfishos-qtquick2app` qmake template (harbour-swell); empty app builds for aarch64.
- [x] Spike: loopback QTcpServer + QUdpSocket QtTest via `make check` in the aarch64 target; record the result (i486 fallback only as a recorded deviation).
- [x] `lsdp::parse` and `Discovery` with unit tests (AC2 cases).
- [x] `parseStatus` / `parseSyncStatus` / `parseVolume` with unit tests on synthetic fixtures.
- [x] `PlayerClient` (shared NAM, long poll, one-second rule, backoff, commands, /SyncStatus policy) and `artworkUrl()`, with in-process fixture tests (AC3).
- [x] `PlayerModel` with discovery merge, manual hosts and tests.
- [x] MVP parsers: `Status.prid`, `parsePresets`, `parseCaptureInputs` with tests (40c5af5).
- [x] MVP client/model: encoded-query gate, presets/inputs cache, `playPreset`/`playInput`, `refreshInputs`, `<error>` bodies via `commandFailed`; `PlayerModel` `currentId`/`current`/`pollAll`, remembered player with `nodeId`; byte-exact fake-player request log; tests (AC3).
- [x] AC4 spike: QML engine test on `minimal`: an `ApplicationWindow` root with `Nemo.Configuration`, a negative control, one shared type-registration function.
- [x] Players page, add-player dialog and Home (presets and inputs), with Nemo.Configuration persistence and the start-state rule.
- [x] Now Playing as an attached page, with landscape layout.
- [x] Docked mini-player, cover and About; `allowedOrientations` on every page.
- [x] AC4 engine test over all QML files.
- [ ] Build the RPM and run `sdk-harbour-rpmvalidator` (AC5); record deviations.
- [ ] Complete the separate result review against the spec (AC6).
- [ ] Prepare a change for maintainer review with evidence in `evidence/001/`.
