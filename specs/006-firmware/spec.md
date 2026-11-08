# Feature specification — 006 firmware updates

Status: draft 2026-11-08, for Misja's decision and the scope/design review.
No implementation and no UI yet.

## Owner

- Accountable owner: root (primary assistant session).
- Decision owner: Misja Hoebe (@misja).
- Issue: to be opened once Misja approves the scope.

## Problem

Firmware updates are the one thing Misja still needs the official BluOS
Android app for. Player settings and alarms were 006's original topic;
they are out of this spec (see "Out of scope").

## Facts (2026-11-08)

- **The published API** (BluOS Custom Integration API v1.7) has no settings,
  alarms or firmware updates: §9 only offers a soft reboot (`POST /reboot`).
  `/Status` reports an alarm that is playing and the sleep timer.
- **The official app**, as observed by azzurro (jzbz/azzurro 10cce29,
  `docs/protocol.md`, "Firmware upgrades"):
  - It polls `GET :11000/upgrade?upgrade=check`. This is non-destructive,
    and the app sends it every 15 s while waiting.
  - It does not install the update itself: it shows the player's own page
    `http://<player>/upgrade` (port 80) in a web view, and the user acts on
    what the player draws.
  - `upgrade=this` starts an update directly. Sending it during a running
    update risks the player.
- **On Misja's NAD C 379** (firmware 4.18.18; read-only, with his approval):
  - `GET :11000/upgrade?upgrade=check` answers
    `<upgrade inProgress="false" available="false">check</upgrade>`. There is
    no `version` attribute when no update is offered.
  - `GET http://192.168.1.72/upgrade` is an HTML page titled "Upgrade" (827
    bytes) that says "No update available."
- **Other parts of `/Settings`** (port 11001, observed): alarms, sleep timer,
  network shares, reindex, artwork optimisation and server mode, as
  undocumented forms. Not used here.

## Proposed scope (for decision)

1. **Show that an update is available.** Read `upgrade=check` (read-only)
   for the current player at start and now and then, never during playback
   control. When `available="true"`, Swell shows a notice with the offered
   version where the player sends one.
2. **Update through the player's own page.** The notice opens
   `http://<player>/upgrade` with `Qt.openUrlExternally`, so the Sailfish
   browser shows the player's page and the player runs the update. Swell
   never sends `upgrade=this`. This is what the official app does, and the
   page is the player's own.
3. While `inProgress="true"`, Swell says the player is updating and does not
   count the silence as a fault.

## Out of scope

- Starting an update from Swell (`upgrade=this` / `all`).
- Player settings and alarms through the undocumented `/Settings` and
  `/Alarms`. They stay in the official app or the player's own pages,
  following the principle not to build own solutions where no standard
  exists.

## Open questions for Misja

- **Q1:** is the read-only `upgrade=check` acceptable? It is undocumented
  but non-destructive, and it is the only way to know about an update
  without the official app.
- **Q2:** where should the notice show: Home, the player list, or both?
  This is agreed on the canvas before building.
