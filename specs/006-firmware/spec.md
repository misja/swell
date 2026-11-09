# Feature specification — 006 firmware updates

Status: draft 2026-11-09, for the scope/design review. No implementation and
no UI yet; the UI is agreed on the design canvas before it is built.

## Owner and linked issue

- Accountable owner: root (primary assistant session).
- Decision owner: Misja Hoebe (@misja).
- Issue: opened after the scope/design review.
- Review: a separate agent reviews scope and design before implementation,
  and the result before the PR. A firmware update is release-relevant for
  the user's device, so the result review is required.
- Scope agreed by Misja (2026-11-09):
  - firmware updates only;
  - the read-only `upgrade=check` is acceptable (Q1);
  - the update runs in the app, in a web view (2026-11-09).

## Problem

Firmware updates are one of the things that keep Misja on the official BluOS
Android app. Swell should tell the user when a player has an update and let
them install it, without leaving the app and without Swell itself driving
the update.

## Facts (2026-11-08/09)

- **The published API** (BluOS Custom Integration API v1.7) has no firmware
  update. §9 only has a soft reboot (`POST /reboot`).
- **The official app**, as observed by azzurro (jzbz/azzurro 10cce29,
  `docs/protocol.md`, "Firmware upgrades"; not documented by Lenbrook):
  - `GET :11000/upgrade?upgrade=check` is non-destructive. The app polls it
    every 15 s only while it waits for a player to come back.
  - It answers `<upgrade inProgress="…" available="…" version="…">check</upgrade>`.
    `version` names the offered release.
  - The app does not drive the update: it shows the player's own page
    `http://<player>/upgrade` (port 80) in a web view.
  - `upgrade=this` starts an update directly. A second start over a running
    one can make the player unusable. The update cannot be aborted.
  - During an update `/SyncStatus` answers with the root `UpgradeStatusStage1`
    or `UpgradeStatusStage2` instead of `SyncStatus`. The player is silent
    for a while, as it reboots. One observed run took about 4.5 minutes.
- **On Misja's NAD C 379** (firmware 4.18.18; read-only, with his approval):
  - `GET :11000/upgrade?upgrade=check` →
    `<upgrade inProgress="false" available="false">check</upgrade>`. There is
    no `version` attribute when nothing is offered.
  - `GET http://192.168.1.72/upgrade` is an HTML page titled "Upgrade" that
    says "No update available."
- **Harbour** (validator in the SDK target 5.1.0.11):
  - `allowed_qmlimports.conf:13` allows `Sailfish.WebView 1.0`;
  - `allowed_libraries.conf:25` allows `libsailfishwebengine`;
  - `allowed_requires.conf:36` allows `sailfish-components-webview-qt5`;
  - `allowed_permissions.conf:13` allows the `WebView` permission.

  QtWebKit and QtWebEngine are not allowed.
- **Swell today:**
  - `/Status` with another root element makes a player NotAPlayer.
  - A `/SyncStatus` that is not `SyncStatus` is ignored.
  - An offline player is retried with backoff.

## Scope

1. **Check:**
   - The library reads `upgrade=check` for a player: when it comes online,
     and then at most once an hour while it is polled.
   - It never sends `upgrade=this` or `all`.
   - It exposes, typed, whether an update is available, the offered version
     when sent, and whether one is in progress.
2. **Notice:** when the current player has an update, the app says so, with
   the version when known. Where it shows is agreed on the canvas.
3. **Update in the app:**
   - The notice opens a page with a Sailfish WebView on
     `http://<player>/upgrade`, the player's own page. The user starts and
     follows the update there, as in the official app.
   - The app adds the `WebView` Sailjail permission and requires
     `sailfish-components-webview-qt5`.
4. **While updating:**
   - When `inProgress="true"`, or `/SyncStatus` answers with an
     `UpgradeStatusStage*` root, the player shows as updating, not as "not a
     BluOS player" or broken.
   - The player's silence during the update is expected.
   - When it answers as a player again, it is online, with its new version.

## Room to grow (Misja, 2026-11-09)

Player settings are expected to follow (Q0). This design should leave room
for them without building anything for them now:
- **Entry point:** the place where firmware lives should be one that can
  grow into "Player settings" for the current player. For example, a
  "Player" or "Settings" page per player, where firmware is the first item
  and settings can be added later. It should not be a one-off firmware
  screen that would have to move.
- **Web page:** the in-app web page shows any page the player itself serves
  (`/upgrade` now; its own WiFi and network-share pages are listed in the
  settings tree). It is not tied to firmware.
- **Agree on the canvas:** the entry point and that page's structure are
  agreed on the canvas with the future settings in view.

## Out of scope

- Starting, retrying or aborting an update from Swell itself.
- Player settings and alarms (the undocumented settings tree on port 11001
  that the NAD showed on 2026-11-08). They are a separate decision, Q0,
  recorded in STATUS.md.
- Updating players other than the current one, and players in a fixed group
  (the zone-member addressing azzurro describes).

## Acceptance criteria

1. The library reads `upgrade=check` on coming online and at most hourly,
   and never sends anything else to `/upgrade`. Tests against the fake
   player cover:
   - available with a version;
   - not available;
   - in progress;
   - no answer;
   - the request targets.
2. A `/SyncStatus` with `UpgradeStatusStage1` or `UpgradeStatusStage2` marks
   the player as updating, not as NotAPlayer. After the update it is online
   again. A test drives this sequence on the fake player.
3. With an update available, the app shows the notice where agreed on the
   canvas. The notice opens the in-app web view on the player's
   `/upgrade` page.
4. The SDK build and the Harbour validator pass with the new import,
   requirement and permission, for aarch64 and armv7hl.
5. A device check by Misja covers:
   - the notice and the web view against the NAD, showing "No update
     available" today;
   - the permission prompt on first start after the upgrade of Swell.

   A real firmware update is checked when one is offered, and recorded as
   not verified until then.
6. A separate result review before the PR.
