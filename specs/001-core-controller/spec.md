# Feature specification — 001 core controller

Status: scope revised 2026-11-06 for the agreed MVP and navigation structure
(Home, presets, inputs, landscape). This revision needs a separate scope
review before UI implementation. The protocol library parts reviewed earlier
are unaffected.

## Owner and linked issue

- Accountable owner: root (primary assistant session).
- Decision owner: Misja Hoebe (@misja).
- Scope reviewers: separate agents `scope_review` (first scope) and
  `scope_review_2` (MVP revision), both 2026-11-06, verdict "approve with
  changes". Their findings are addressed below (see STATUS.md).
- Result reviewer: a separate agent reviews the resulting changes before
  maintainer review.
- Linked issue: https://github.com/misja/swell/issues/1 (private repository).
- UI design reference: the design canvas linked from issue #1. Its notes record
  the maintainer's UI decisions of 2026-11-06.

## Problem and intended users

Owners of BluOS players (Bluesound, NAD, PSB) who use Sailfish OS have no native
controller; searches of OpenRepos, Harbour and GitHub on 2026-11-06 found none.
The official app is Android-only and needs AppSupport. A native Silica app
should cover the everyday loop: find the players on the local network, pick
something quick to play (a preset or an input), see what is playing, and control
playback and volume. This first version is a complete remote control, not yet a
music browser.

## Scope and exclusions

Silica/QML UI plus a thin Qt 5.6 C++ layer, using only facilities present in
the target. Rust and azzurro's `bluos` crate are not used: the SDK ships
Rust 1.75, while azzurro requires 1.98 and edition 2024.

This first increment (the MVP) includes:

1. Player discovery via LSDP (UDP 11430 broadcast), plus manual entry of
   `host[:port]`. Only player classes 0x0001, 0x0003, 0x0006 and 0x0008 are
   listed (API v1.7 appendix, class ID assignments).
2. **Players page**: every found and manual player with its now-playing line or
   state. Tapping one makes it the current player. Manual players can be
   removed. The current player is remembered and is used again at the next
   start.
3. **Home page** of the current player. The app starts here when a current
   player is remembered; otherwise it starts on Players. If the current player
   is offline, Home shows that state with a hint to pick another player. Home
   shows:
   - the player's presets (API §6: `/Presets`; play with `/Preset?id=`),
     fetched on first contact and refetched when `prid` in `/Status` changes;
     each player keeps its own, so a player switch shows its presets at once. A player without presets shows a placeholder
     (presets are created in the official app). A preset is played by its
     `id`, because ids are not sequential;
   - its inputs (API §11.1: `/RadioBrowse?service=Capture`; play with
     `/Play?url=`, passing the already percent-encoded `URL` attribute without
     re-encoding). Only local hardware inputs (items with `inputType`) are
     shown. Hub inputs and cloud entries, such as Spotify, which belongs to an
     account, are left out. Known limitation: inactive inputs may be missing
     from this list (API §11.2), and §11.2 is not used.
   Pulley: Players, About.
4. **Now Playing** as the page attached to Home (swipe left). It shows cover
   art, the `title1`–`title3` lines, progress (display only, no seeking),
   play/pause, next/previous, volume and mute. The volume control is hidden
   when the player reports a fixed volume (`volume=-1`). It is updated through the `/Status` long poll (etag +
   timeout), not a polling loop.
5. A **docked mini-player** on Home and Players (artwork, title, play/pause).
   Tapping it opens Now Playing. It is a plain Silica `DockedPanel`.
   `Sailfish.Media`'s `MediaPlayerControlsPanel` was examined and not reused:
   it always shows a seek slider, has no artwork/title slot and opens only
   while playing.
6. **Cover**: current track, with play/pause and next cover actions.
7. **Landscape** on every page, using Silica's orientation handling. Pages
   default to portrait, so each page sets `allowedOrientations`. Now
   Playing shows the artwork beside the controls; lists stay lists.
8. **About** page with the privacy statement.

Agreed structure for later features (design canvas, issue #1). Each needs its
own spec:
- 002: the Queue page, attached to Now Playing.
- 003: MPRIS lock-screen controls (needs the `Audio` permission).
- 004: a generic Browse page for the player's `/Browse` hierarchy, from Home:
  library, services, playlists, radio and search. Whether Home's sections then
  come from the top-level `/Browse` is decided in 004. 001 keeps play actions
  generic ("send this play path"), so the source of the inputs can change
  later.
- 005: grouping, as its own page opened from Now Playing.
- 006: player settings and alarms (to be discussed).
- Later: My Stations (custom radio URLs stored on the phone), a "Track details"
  pulley item on Now Playing, hardware volume keys (Harbour/Sailjail
  feasibility unverified) and a firmware notice.

Out of scope altogether: anything behind a BluOS account, such as linking
music services. Harbour submission, OBS/Chum publication and device
installation need separate approval.

## Required behavior

- All network I/O is asynchronous; the UI thread never blocks.
- The plan records the reason each part is in C++ or QML. Only LSDP strictly
  needs C++ in Qt 5.6. QML `XMLHttpRequest`/`XmlListModel` (allowed by the
  Harbour validator) is the baseline alternative for HTTP/XML. C++ needs a
  stated reason such as long-poll timeout/abort control or QtTest coverage.
- Long polling follows API v1.7 §2: one long poll per player on `/Status`,
  using the `etag` from the previous response, and never two consecutive
  requests for the same resource less than one second apart. Without a long
  poll, the client polls at most once every 30 seconds.
- The long-poll HTTP timeout is greater than the requested poll `timeout`. If a
  player is unreachable, it is shown as offline and retried with backoff, with
  no user action needed.
- Cover art follows redirects to port 11004. Qt 5.6.3 has
  `FollowRedirectsAttribute`, and the QML `Image` element follows redirects
  itself. HTTPS cover art uses the system CA store, with no exceptions.
- Unknown XML elements and attributes are ignored. Malformed or truncated input
  never crashes the app.
- No accounts, credentials, telemetry or analytics. Local state is limited to
  manual hosts and the current player. The About page shows a short privacy
  statement saying so, as the API Use Policy requires.
- Lenbrook marks: no BluOS name or logo in the app name or icon. Descriptions
  use only the policy's accepted form "Swell (works with BluOS)" and do not
  suggest endorsement.
- Sailjail permission: `Internet` only.
- Presets and inputs show what the player reports. Their images go through the
  same artwork resolution as the cover art: player-relative images get
  `followRedirects=1`, and absolute URLs on other hosts are used as given. A
  failed play is reported in the UI, and the long poll then shows the actual
  state.
- Pulley menus stay short, so they also fit in landscape (Sailfish design
  guideline, navigation: "below five" options,
  https://sailfishos.org/design/navigation, checked 2026-11-06).
- Background polling: only the current player keeps its long poll while the
  app is in the background, because the cover shows it. Other players are
  polled only while the Players page is visible. A non-current player shows
  its last known state; when the Players page appears, every player is
  refreshed.
- A failed preset or input play is an HTTP or network error, or an `<error>`
  body. Success is a `<loaded>` or `<state>` response (API §6.2, §11.1).
- UI strings are translatable using the Sailfish convention; the mechanism is
  chosen in the plan.

## Acceptance criteria and completion condition

1. Builds with `sfdk` (SDK 3.13.5) for the approved build target
   (proposed: SailfishOS-5.1.0.11-aarch64; see decisions).
2. Parser unit tests pass on synthetic fixtures. They cover:
   - LSDP announce parsing, including truncated packets, unknown message types,
     the app's own echoed `Q` packet, duplicate announcements, an address
     change for the same node id, Delete messages, announcements split across
     messages and the class filter;
   - `/Status` and `/SyncStatus`, including unknown elements and malformed XML;
   - `/Presets` and the `/RadioBrowse?service=Capture` input list, including
     unknown elements, items without `inputType`, nested Hub categories and
     malformed XML.
3. A minimal fixture server in the test suite (Python stdlib `http.server` or
   Qt `QTcpServer`, decided in the plan) passes these integration checks:
   - it serves synthetic XML and holds the long poll until the etag changes or
     the timeout expires;
   - it answers `/Artwork` with a redirect to a second port;
   - it logs requests;
   - it shows that transport, volume and mute issue the expected requests;
   - it shows that no resource is requested twice within one second;
   - an etag-unchanged timeout causes no UI change;
   - when the server is stopped and restarted, the player goes offline and
     recovers;
   - a manual host that is not a BluOS player is reported clearly;
   - presets and inputs are fetched, and playing one sends `/Preset?id=` or
     `/Play?url=` with the input URL not encoded twice;
   - a `prid` change in `/Status` refetches `/Presets`.
4. Every QML file compiles, with syntax, imports and types resolved, in the
   build target. This is checked by a QtTest engine test on the `minimal`
   platform, which creates a `QQmlComponent` per file without instantiating
   windows. The target has no `qmllint` and no offscreen platform, so runtime
   binding warnings and rendering are recorded as not checked.
5. The RPM passes `sdk-harbour-rpmvalidator` 1.110 (present in the target), or
   deviations are listed with reasons. This is a compliance check, not a
   submission.
6. A separate agent reviews the result against this spec before maintainer
   review.

Real-device checks are not part of completion unless the maintainer approves a
device installation; otherwise they are recorded as not performed. These are
LSDP broadcast over Wi-Fi under Sailjail, suspend/resume and network change,
silence during a firmware upgrade, and the look and gestures of the pages in
portrait and landscape.

## Affected targets and compatibility

Inspected 2026-11-06 in the `sailfish-sdk` LXC build engine (SDK 3.13.5,
target SailfishOS-5.1.0.11-aarch64):
- Qt 5.6.3 (qtcore/qtnetwork/qtdeclarative);
- sailfishsilica-qt5 1.2.154 and libsailfishapp 1.2.17;
- sailjail 1.1.28 and sailjail-permissions 1.1.21;
- sdk-harbour-rpmvalidator 1.110; OpenSSL 3.5.6.

Release target `sailfishos-5.2.0.18-aarch64` (the workspace registry's only
target). The build target is SailfishOS-5.1.0.11-aarch64, approved by Misja on
2026-11-06 as the mapping for that release. The workspace manifest still lists
`sdk_build_target: null`; that is the workspace's own release record and is
not changed by this project. Evidence that the app runs on 5.2 is a separate
runtime check later. armv7hl and i486 are not acceptance targets.

The players use the BluOS control API on port 11000.

## Evidence, assumptions and unresolved questions

Facts (2026-11-06; validator files in
`/usr/share/sdk-harbour-rpmvalidator/` in the target):
- `allowed_qmlimports.conf:44` allows `Amber.Mpris 1.0`, `:64` allows
  `QtQuick.XmlListModel 2.0` and `:110` allows `Nemo.Configuration 1.0`.
- `allowed_requires.conf:62-63` allows `amber-qml-plugin-mpris`; the C++
  `libambermpris` is not in `allowed_libraries.conf`.
- `allowed_permissions.conf` allows `Audio` and `Internet`.
- `Internet.permission`: `ignore net none` and `protocol inet,inet6,netlink`.
  UDP broadcast is therefore expected to work but has not been tested on a
  device.
- `Audio.permission` grants `dbus-user.own org.mpris.MediaPlayer2.*` and broader
  audio access (relevant for 003).

Protocol sources:
- Normative: Lenbrook "BluOS Custom Integration API" version 1.7 (revision
  dated 04/09/2025; PDF created 2025-06-12),
  https://bluos.io/wp-content/uploads/2025/06/BluOS-Custom-Integration-API_v1.7.pdf,
  sha256 87fd68fc2bd6584c07f13307c0a8915dace50b74c913e216856997e9cb543c36,
  downloaded 2026-11-06. It covers everything this feature uses:
  - §2 `/Status` and `/SyncStatus` with long polling;
  - §3 volume and mute;
  - §4 play, pause, skip and back;
  - §6 presets and §11.1 input selection;
  - image attributes with `followRedirects=1`;
  - the Appendix's LSDP packet format, timing and class IDs.
  The document must not be reproduced, so it is not committed. Synthetic
  fixtures are written by the project and are not copied from its examples.
- Supporting only: azzurro `docs/protocol.md`, commit 9375da6 (MIT), for
  behavior it observed on BluOS 4.16.x. Its "Transcribed" items, which were
  read from the official controller, are not used.

API Use Policy (in the v1.7 document), points needing maintainer judgement:
- It requires a privacy policy; covered above by the About statement.
- It restricts use of the marks; covered above by the naming rule.
- It forbids reverse engineering. Using only the official document avoids
  relying on reverse-engineered details.
- It forbids using the APIs "to compete with our Service". It is unclear
  whether a third-party controller counts; this is a maintainer risk judgement
  and not legal advice.
- It mentions application approval before listing on Lenbrook's sites. Whether
  any registration is needed for an unlisted app is unknown.

Decisions (Misja, 2026-11-06):
- Build mapping: SailfishOS-5.1.0.11-aarch64 for release target
  sailfishos-5.2.0.18-aarch64.
- License: MIT (SPDX `MIT`; no copyleft, so linking Silica raises no license
  conflict).
- Test endpoint: azzurro's Rust fake-player may be used locally if it stays
  limited and isolated. A concrete isolation proposal goes in the plan and
  needs explicit approval before any toolchain installation.

- API Use Policy "compete" clause: Misja's judgement is that Swell fills a
  gap, a native Sailfish OS controller, and does not compete with Lenbrook's
  service. azzurro faces the same clause.
- An LXC container for the fake-player and other test work is approved.

- Remote: private GitHub repository misja/swell; issue #1 is on the Sailfish
  project board.
- UI (2026-11-06): start on Home of the last chosen player; source and quality
  are not shown on Now Playing (a details view later); grouping goes on its own
  page from Now Playing; landscape is supported everywhere; My Stations is
  stored locally; hardware volume keys are wanted, subject to feasibility.
- MVP scope (2026-11-06): presets and inputs on Home are in 001; the rest
  follows the agreed structure.

Still open: player settings (006), to be discussed.
