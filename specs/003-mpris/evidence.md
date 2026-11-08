# Evidence — 003 MPRIS lock-screen controls

## Build and tests (2026-11-08)

Branch 003-mpris, SDK target SailfishOS-5.1.0.11-aarch64.

- **`make check`:** all nine suites pass: spike 5, lsdp 18, xml 30,
  client 29, model 26, qml 17 (`qml/Mpris.qml` compiles), qmlruntime 16 (15 before
  `mprisWithoutAPlayer` from the result review),
  queue 28, browse 26. No compiler warnings.
- **qmlruntime, four MPRIS tests** (spec AC5): `mprisFollowsTheStatus`,
  `mprisMetadataAndPosition`, `mprisRequestsReachThePlayer` and
  `mprisOffline`. They run with `DBUS_SESSION_BUS_ADDRESS` pointing nowhere,
  so `MprisPlayer` never registers.
  - Amber 1.2.9 drops a duration of 0. JavaScript then reads it as empty,
    which the tests expect.
- **Negative control:** `canControl: false` in the SDK copy only. Two tests
  then failed (`mprisFollowsTheStatus` and `mprisOffline`: all can* read
  false). The copy was restored and compared identical to the source.
  - Only two failed because the request tests emit the signals directly.
    They bypass amber's own D-Bus checks on canControl and can*
    (mprisplayer.cpp:132-230); the device check covers that path.
- **RPM:** harbour-swell-0.3.0-14.aarch64.rpm, sha256
  bfe483191c5bfd1da22445170fed12fa8f63bf51955838705ca31af01d0af9d9 (rebuilt
  after the result review; the first build was 4b948e9e…).
  - Requires `qml(Amber.Mpris)`.
  - Its desktop file has `Permissions=Internet;Audio`.
- **Phone:** 0.3.0-14 installed 2026-11-08; the phone has amber-qml-plugin-mpris
  1.2.10-1.10.1 (the target has 1.2.9).
- **`sfdk check -s harbour`:** validation succeeded. Its one warning, "file
  is not stripped", was already in the 001, 002 and 004 validator logs.
- Translations unchanged.
- **Same branch:** Misja's corrected icon (the sound bars centred, a
  translate of 1.005). The PNGs were rendered again with QSvgRenderer at
  86, 108, 128 and 172 px.

## Device check (2026-11-08)

Misja checked 0.3.0-14 on the Xperia 10 II (amber-qml-plugin-mpris
1.2.10). The phone was on Sailfish OS 5.2.0.20 at install and reported
5.2.0.22 after an OTA the same day; which one the check ran on was not
recorded and reported "werkt" (works). He was given
the AC6 list before the check:
- the permission prompt;
- the lock screen and the events view;
- controls while locked, and changes from the official app;
- local audio while BluOS plays, and the headset key;
- Audio revoked, where Settings allows it.

The report was not broken down per item.

## Review findings

Scope/design review, 2026-11-08 (approve with changes):

| ID | Finding | Disposition |
|---|---|---|
| H1 | Swell takes the lock screen and headset keys while BluOS plays | Q1, agreed by Misja; device check AC6. |
| H2 | canControl, hasShuffle, hasLoopStatus lock after the first GetAll | Literals in Mpris.qml, with a comment. |
| H3 | Service name gets `.instance<pid>`; empty-name trap | Spec facts corrected; serviceName is a literal. |
| M1 | Position as a binding; -1 when unknown | Set in `onPositionRequested`, clamped to 0. |
| M2 | No `app` inside Mpris.qml | `player` property; raise handled in harbour-swell.qml. |
| M3 | Metadata with no player or offline | Cleared; tested in `mprisOffline`. |
| M4 | Request mapping and guards | playPause by `playing`; all handlers check online; stop ignored. |
| M5 | Q2 incomplete (upgrade prompt, refusal) | Q2 extended; Sailjail source checked (spec). |
| L1 | Plain strings for the metadata | `String(artworkUrl)`. |
| L2 | canRaise needs no separate check | Kept, no separate check. |
| L3 | Q3 to a device check | AC6. |
| L4 | "connecting" maps to Stopped | As specified; tested. |

Result review, 2026-11-08 (approve with changes; no High findings):

| ID | Finding | Disposition |
|---|---|---|
| M1 | STATUS.md stale (permission decision, no 003 handoff) | Updated. |
| M2 | Spec status and Q1/Q2 wording stale | Updated; "Decisions (Q1, Q2)". |
| L1 | No-player case (AC3) and metadata guards untested | `mprisWithoutAPlayer`, a second Mpris with no player. |
| L2 | Absolute request counts depend on test order | Baselines taken before each request. |
| L3 | Why the negative control failed only two tests | Noted above. |
| L4 | No row for this review | This table. |
