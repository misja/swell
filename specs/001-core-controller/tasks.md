# Feature tasks — 001 core controller

Linked issue and plan: no issue yet (STATUS.md handoff); plan.md in this directory.
Use this file for bounded implementation steps, not a second status board.

- [ ] Scaffold from the SDK `sailfishos-qtquick2app` qmake template (harbour-swell); empty app builds for aarch64.
- [ ] Spike: loopback QTcpServer + QUdpSocket QtTest via `make check` in the aarch64 target; record the result (i486 fallback only as a recorded deviation).
- [ ] `lsdp::parse` and `LsdpDiscovery` with unit tests (AC2 cases).
- [ ] `parseStatus` / `parseSyncStatus` / `parseVolume` with unit tests on synthetic fixtures.
- [ ] `PlayerClient` (shared NAM, long poll, one-second rule, backoff, commands, /SyncStatus policy) and `artworkUrl()`, with in-process fixture tests (AC3).
- [ ] `PlayerModel`; QML pages (player list, add player, Now Playing, About/privacy); cover page and actions; Nemo.Configuration persistence.
- [ ] Build the RPM and run `sdk-harbour-rpmvalidator` (AC4); record deviations.
- [ ] Optional, non-gating: Layer B against demo-player (needs the swellbr0 / sailfish-sdk NIC approval).
- [ ] Complete the separate result review against the spec (AC5).
- [ ] Prepare a change for maintainer review with evidence in `evidence/001/`.
