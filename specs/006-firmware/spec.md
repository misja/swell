# Feature specification — 006 firmware updates

Status: implemented 2026-11-09 on branch 006-firmware; the scope/design review
and the result review (both approve with changes) are applied. UI agreed on the
design canvas, page "Firmware (006)". Revised 2026-11-25 (draft): Swell starts
the update itself; see "Revision 2026-11-25" below, which replaces scope
item 3 and the first point of "Out of scope".

## Owner and linked issue

- Accountable owner: root (primary assistant session).
- Decision owner: the maintainer.
- Issue: misja/swell#28.
- Review: a separate agent reviews scope and design before implementation,
  and the result before the PR. A firmware update is release-relevant for
  the user's device, so the result review is required.
- Scope agreed by the maintainer (2026-11-09):
  - firmware updates only;
  - the read-only `upgrade=check` is acceptable (Q1);
  - the update runs in the app, in a web view (2026-11-09).

## Problem

Firmware updates are one of the things that keep the maintainer on the official BluOS
Android app. Swell should tell the user when a player has an update and let
them install it, without leaving the app and without Swell itself driving
the update.

## Facts (2026-11-08/09)

- **The published API** (BluOS Custom Integration API v1.7) has no firmware
  update. §9 only has a soft reboot (`POST /reboot`).
- **The official app**, as observed by azzurro (jzbz/azzurro 10cce29,
  `docs/protocol.md`, "Firmware upgrades"; not documented by Lenbrook):
  - `GET :11000/upgrade?upgrade=check` is non-destructive. The app polls it
    every 15 s while waiting for a player to come up, with a 60 s timeout.
    Its parser reads only `inProgress` and `available`.
  - It answers `<upgrade inProgress="…" available="…" version="…">check</upgrade>`.
    `version` names the offered release.
  - The app does not drive the update: it shows the player's own page
    `http://<player>/upgrade?noheader=1` (port 80) in a web view.
  - `upgrade=this` starts an update directly. A second start over a running
    one can make the player unusable. The update cannot be aborted.
  - During an update `/SyncStatus` answers with the root `UpgradeStatusStage1`
    or `UpgradeStatusStage2` instead of `SyncStatus` (seen on /SyncStatus
    only; /Status during an update is not observed). One Powernode run:
    silent 72–88 s, Stage1 at 88 s, Stage2 at 104 s, SyncStatus again at
    265 s. The official app holds no long poll then and reads /SyncStatus
    every 5 s. azzurro saw a `version` attribute on SyncStatus afterwards
    (not in the API).
- **On a NAD C 379** (firmware 4.18.18; read-only, with the maintainer's approval):
  - `GET :11000/upgrade?upgrade=check` →
    `<upgrade inProgress="false" available="false">check</upgrade>`. There is
    no `version` attribute when nothing is offered.
  - `GET http://<player>/upgrade` is an HTML page titled "Upgrade" that
    says "No update available."
- **Harbour** (validator in the SDK target 5.1.0.11):
  - `allowed_qmlimports.conf:13` allows `Sailfish.WebView 1.0`;
  - `allowed_libraries.conf:25` allows `libsailfishwebengine`;
  - `allowed_requires.conf:36` allows `sailfish-components-webview-qt5`;
  - `allowed_permissions.conf:13` allows the `WebView` permission.

  QtWebKit and QtWebEngine are not allowed. `qml(Sailfish.WebView)` is not
  an allowed requirement; the package name is.
- **Sailfish WebView in the target** (SDK documentation
  sailfish-webview260616.qch; package sailfish-components-webview-qt5
  1.6.9, in the jolla repo, not installed in the target):
  - The documented pattern is `WebViewPage { WebView { url: … } }`.
  - `WebEngine` initialises itself in QML; there is no `main()` change.
  - Apps link against qtmozembed: `PKGCONFIG += qt5embedwidget`, with
    `BuildRequires: pkgconfig(qt5embedwidget)` (qtmozembed-qt5-devel).
  - Gecko (libxul) runs in the app's process when the page opens. Because
    the binary links qtmozembed, libxul (about 109 MB) is also mapped and
    relocated at every app start (result review, verified with readelf). The
    effect on start-up time and memory is measured in the device check.
- **The WebView permission** (`/etc/sailjail/permissions/WebView.permission`):
  "Web content" / "Viewing web content". It also opens a few paths
  (/usr/share/mozilla, /var/tmp, ~/Desktop, …) and D-Bus to pacrunner and
  the transfer engine.
- **Swell today:**
  - `/Status` with another root element makes a player NotAPlayer.
  - A `/SyncStatus` that is not `SyncStatus` is ignored.
  - An offline player is retried with backoff.

## Scope

1. **Check (current player only):**
   - The library reads `upgrade=check` (60 s deadline) while status polling
     is on: when the player comes online, hourly, when Settings opens, and
     when the update page is left or covered.
   - It never sends `upgrade=this` or `all`, nor `slave`/`port` parameters.
   - It exposes, typed, whether an update is available, the offered version
     when sent, and whether one is in progress.
   - No answer, 404 or a malformed document keep the previous value and do
     not touch availability. So a failed check after an update keeps the
     notice until the next check.
   - The current player only: the player list polls every player's
     status, but PlayerModel switches the checks off for all others.
   - No notice for a secondary in a fixed group.
2. **Notice and entry point** (canvas "Firmware (006)", agreed by the maintainer
   2026-11-09):
   - When the current player has an update, Home shows one line under the
     presets with the version when known. Home only, not Now Playing.
   - Home is the player, so Home's pulley gets "Settings": the current
     player's settings page, with model, firmware version and a "Firmware"
     section (the check's answer, a "Start update" button only while an
     update is on offer, or "Updating"; the maintainer, 2026-11-09). Later player
     settings join it as further sections.
   - The notice opens the player's update page directly (the maintainer, 2026-11-09,
     after the device check: the extra step through Settings is not wanted).
   - The update page has the player's name as header.
3. **Update in the app:**
   - The Home notice and the Settings button open a `WebViewPage` with a `WebView` on
     `http://<player>/upgrade?noheader=1`, the player's own page. The user
     starts and follows the update there, as in the official app.
   - The page is only loaded when pushed, so the engine starts only then.
     The library itself is linked (see the Facts above).
   - Links on the player's page can lead elsewhere; the view keeps no
     history (`privateMode`).
   - The app adds the `WebView` permission, requires
     `sailfish-components-webview-qt5`, and builds with qt5embedwidget.
4. **While updating** (an `Updating` availability, not a separate flag):
   - The player enters Updating on `inProgress="true"` or on an
     `UpgradeStatusStage*` root from any /SyncStatus read.
   - When /Status fails or answers another root, a plain /SyncStatus read
     classifies the player before it is marked Offline or NotAPlayer. That is
     one paced read per backoff step (at most every 30 s); an unreachable
     player is marked Offline up to `commandTimeoutMs` (10 s) later.
   - Entered through `inProgress="true"` (the update downloads, the player
     still answers as one), it stays Updating until a check no longer says
     so, or no longer answers.
   - While Updating, no long poll is held, neither on /Status nor on the
     /SyncStatus watch. /SyncStatus is read plainly on the existing backoff,
     and silence is expected.
   - Updating ends only when /SyncStatus answers with a `SyncStatus` root.
     The check then runs again, so the notice clears.

## WebView version (decided by the maintainer, 2026-11-09)

Swell links against qt5embedwidget, as WebView 1.6.9 in the SDK target asks,
and accepts the validator's rpath WARNING. Dropping the link from WebView 1.8
onwards is tracked in issue #32.

## Room to grow (the maintainer, 2026-11-09)

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

## Revision 2026-11-25: Swell starts the update

### Facts (2026-11-25, NAD C 379, firmware 4.18.18; reads only)

- `upgrade=check` answers `available="true"`, `inProgress="false"`,
  `version="4.18.26"`. Swell's notice and Settings show it.
- `http://<player>/upgrade?noheader=1`, the page Swell and the official app
  open, says only "Update available.". It has no button, and a script that
  calls `window.close()` after 5 s. So the user cannot start the update in
  Swell's web view.
- Without `noheader` the page adds an "Upgrade Now" button whose `onClick`
  calls `$.get('/upgrade', {upgrade: 'old'})`. The page loads no jQuery, so
  `$` is undefined and the button cannot work in any browser.
- azzurro (jzbz/azzurro 10cce29, MIT; `docs/protocol.md` and
  `crates/bluos/src/client.rs` `start_upgrade_for`) starts an update with
  `GET :11000/upgrade?upgrade=this`, and only after a check answered
  `available="true"` and `inProgress="false"`. That is the precondition the
  official app enforces: "Starting a second upgrade over a running one is
  the way to turn a working speaker into a brick." The answer to `this` is
  "the same document the check answers with" and is not read. azzurro has
  driven updates end to end on a Powernode.
- What a NAD answers to `this`, and whether a check reports
  `inProgress="true"` while it downloads, is not known until the first
  update.

### Scope (replaces scope item 3)

1. **Start:** the client gets one entry point that starts the update of the
   current player, given the version the user confirmed. It sends
   `GET :11000/upgrade?upgrade=this` at most once, and only when all of the
   following hold at the moment of sending:
   - the client runs, polls status, checks upgrades (current player), the
     player is online and not a secondary in a fixed group;
   - it is not Updating, and no start is pending or was sent before (see 2);
   - a check made for this start, after the user confirmed, answered
     `available="true"`, `inProgress="false"`, and the confirmed version.

   Otherwise nothing is sent and the user is told that the update was not
   started. Never `all`, `slave` or `port`.
2. **Once:** before `this` is sent, the client records that it was sent. That
   record is not cleared by an error, a timeout or no answer: those are not
   retried, since a player that has begun rewriting its flash may not answer.
   It is cleared only when the player has visibly updated (it answered with
   an `UpgradeStatusStage*` root, or `/SyncStatus` names another firmware
   version), so a later release can be started. Otherwise a new start needs a
   new app session, and then the check guards it again.
3. **After sending,** the player is Updating at once, through the existing
   rules (no long poll, /SyncStatus read on the backoff). Updating ends when
   the player answers as itself after evidence that it updated (a stage root,
   a period without answer, `inProgress="true"`, or another firmware
   version). Without such evidence within a fixed bound after the start
   (five minutes, as azzurro allows an unanswered start), Updating ends and
   the user is told that the update did not start; the record of 2 stays.
4. **UI** (to agree with the maintainer):
   - "Start update" in Settings and the Home notice open a Silica `Dialog`
     that names the version and says that the player restarts, cannot be
     used for a few minutes, that the update cannot be stopped, and that the
     player must stay switched on. Accepting starts it; the dialog itself
     cannot accept twice.
   - While starting and updating, Settings shows the state and no button;
     Home's notice says that the player is updating.
5. **The web view:** with the update no longer on the player's page, the
   WebView has no use in Swell. Removing it (page, permission, requirements,
   the qt5embedwidget link; issue #32 then lapses) or keeping it for later
   player pages is the maintainer's decision.

### Acceptance criteria (added)

7. Tests against the fake player, each asserting how many `this` requests
   reached it:
   - a start with a fresh check that is available, matches and is not in
     progress sends exactly one `this`, after that check;
   - none when the fresh check is not available, in progress, names another
     version, gives no answer, 404 or malformed XML;
   - none when offline, Updating, not the current player, a fixed-group
     secondary, or not running;
   - two starts in a row, a start while one is pending, and a start after
     one was sent send one in total;
   - an error, a timeout or no answer to `this` are not retried;
   - after sending, the player is Updating without a long poll; it leaves
     Updating on a stage root then `SyncStatus`, and after the bound
     without evidence, with "not started" reported;
   - after a visible update the record is cleared.
8. The QML runtime test: the dialog starts the update once, and the button
   is hidden while starting and updating.
9. Device check by the maintainer: the update to 4.18.26 on the NAD from
   Swell, followed to the end, with what the player answers recorded in
   evidence.md. This is the only update on offer; if it fails, the official
   app updates the player.
10. A separate result review before the PR (a firmware update is
    release-relevant).

## Out of scope

- Retrying or aborting an update from Swell itself (starting it: see the
  revision of 2026-11-25).
- Player settings and alarms (the undocumented settings tree on port 11001
  that the NAD showed on 2026-11-08). They are a separate decision, Q0,
  now #40.
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
5. A device check by the maintainer covers:
   - the notice and the web view against the NAD, showing "No update
     available" today;
   - the permission prompt on first start after the upgrade of Swell.

   A real firmware update is checked when one is offered, and recorded as
   not verified until then.
6. A separate result review before the PR.
