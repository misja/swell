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
- **The settings tree** (undocumented; read on Misja's NAD with GET only,
  2026-11-08). `/Settings` answers 301 to port 11001, and pages are read with
  `?id=<menuGroup id>`. Each `<setting>` has a `class` (text, boolean, list,
  range, dual-range, button, iconlist) and a write `url` on port 11000
  (`/setting`, `/audiomodes`, `/alsa_setting`, `/Name`). None of the write
  URLs were called.
  - **Top level:** alarms, sleep timer, the menu groups Audio, Player and
    Music library (network shares as a web view of the player's port-80
    page; reindex; artwork optimisation; server mode).
  - **Player:** WiFi (web view of the player's page), room name, "Customize
    sources" (`id=capture`), indicator brightness, LCD volume display.
  - **Audio:** speakers, crossover, replay gain, output mode, volume limits,
    balance, tone controls, reset.
  - **Customize sources:** Bluetooth autoplay, and one menu group per input
    (`capture-xdynamic-SourceN`: Optical 1, Optical 2, CD Player, Coaxial 2,
    Phono, Line 1, Line 2, HDMI eARC, Bluetooth). Per input: name, icon, MQA
    pass-through, A/V mode, lip-sync delay, compressed grouped audio, and
    **"Hide source"** (boolean): turning an input on or off.

## Proposed scope (for decision)

1. **Show that an update is available.** Read `upgrade=check` (read-only)
   for the current player at start and now and then, never during playback
   control. When `available="true"`, Swell shows a notice with the offered
   version where the player sends one.
2. **Update through the player's own page, in the app.** The notice opens
   `http://<player>/upgrade` in a page with Sailfish WebView
   (`Sailfish.WebView 1.0`), so the player's page shows inside Swell, as in
   the official app's web view, and the player runs the update. Swell never
   sends `upgrade=this`.
   - Misja prefers staying in the app (2026-11-09).
   - Harbour allows it: `allowed_qmlimports.conf:13` (`Sailfish.WebView
     1.0`), `allowed_libraries.conf:25` (`libsailfishwebengine`),
     `allowed_permissions.conf:13` (`WebView`), and
     `allowed_requires.conf:36` (`sailfish-components-webview-qt5`).
   - It costs one more Sailjail permission (`WebView`, asked at first start)
     and a dependency.
   - The alternative, the Sailfish browser through `Qt.openUrlExternally`,
     needs neither.
   - The same web view could later show the player's own pages listed in
     the settings tree (WiFi, network shares).
3. While `inProgress="true"`, Swell says the player is updating and does not
   count the silence as a fault.

## Out of scope

- Starting an update from Swell (`upgrade=this` / `all`).
- Player settings and alarms through the undocumented `/Settings` and
  `/Alarms`. They stay in the official app or the player's own pages,
  following the principle not to build own solutions where no standard
  exists.

## Open questions for Misja

- **Q0:** settings such as "Hide source" per input are writable only through
  this undocumented tree. Use them (and which), or keep them in the official
  app?

- **Q1:** is the read-only `upgrade=check` acceptable? It is undocumented
  but non-destructive, and it is the only way to know about an update
  without the official app.
- **Q2:** where should the notice show: Home, the player list, or both?
  This is agreed on the canvas before building.
