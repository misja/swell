# Session handoff

The maintainer's working handoff between sessions. Contributors: see
CONTRIBUTING.md.

Updated: 2026-11-08 (feature 003, MPRIS). Decision owner: Misja Hoebe (@misja).

Current owner / next owner / decision owner: root / root (next session) / Misja.

Now (2026-11-08): feature 003 (MPRIS lock-screen controls, issue #13) on
branch 003-mpris: specs/003-mpris/{spec,plan,tasks,evidence}.md. Scope/design
review and result review done (approve with changes, applied). RPM 0.3.0-14
built and validated; the branch also carries Misja's centred icon. Device
check by Misja passed (2026-11-08). Next: the PR. Merged: #12 (LSDP seed,
29a8e66) and #14 (binary fully stripped, d90f50e).
007 (#8) merged in PR #11 (6ddda82).

Objective and acceptance criteria: feature 004 (browse, issue #7) on branch
004-browse: specs/004-browse/{spec,plan,tasks}.md. UI agreed on the canvas
(B1–B9, N1–N5). Separate reviews done and findings fixed (scope/design,
result, navigation, covers). Device check by Misja passed on 0.3.0-12
(2026-11-08), including navigation (decision 12) and cover placeholders
(decision 13). App icon (#3): Misja's sound-bar design, source
icons/harbour-swell.svg, accepted on the device. 004 merged (PR #9, f48abda,
2026-11-08); #7 and #3 closed. Now: issue #8 on branch 007-bluos-library,
specs/007-bluos-library. API and C/C++ check done; scope/design review and
re-check done (both approve with changes, applied); Misja chose option 3
(typed library, QML shaping in src/). Built: runtime QML test, typed
interfaces, A1-A3, C2; tests, RPM 0.3.0-13 and validator pass. Result review
applied; Misja's device check passed (2026-11-08). PR open. Decisions: browse reads unpaced; no recently played
and no extra state in Swell (published API only); no header per letter.
Issue #8 (typed bluos/ interfaces) follows 004. Workspace principles 1.2.0
(Sailfish UI check in reviews) merged (sailfish-workspace#19, 13985f2).
002 (queue) done (PR #5, #6); 001 done (PR #2).
- Spec 001: specs/001-core-controller/spec.md, criteria AC1–AC6.
- Shared conventions PR https://github.com/misja/sailfish-workspace/pull/18
  (principles 1.1.0).
- The UI design canvas and decisions are linked in issue #1.

Artifacts and evidence:
- bluos/ is the protocol library (namespaces BluOS, Lsdp).
- src/ holds the app stub and QML type registration; qml/ the pages.
- tests/ holds spike, lsdp, xml, client, model, qml and queue.
- translations/ holds English (source) and Dutch (complete).
- specs/<feature>/evidence.md holds each feature's results, reviews and
  device checks (001: 0.1.0-7, 002: 0.2.0-4, 004: 0.3.0-12). Raw build and
  test logs stay local in evidence/ (ignored by Git since 2026-11-08).
- Device: the Xperia 10 II (Sailfish OS 5.2.0.22 since the OTA, 2026-11-08;
  before that 5.2.0.20) runs 0.3.0-14, against a NAD
  C 379.
- Device facts (2026-11-06): `/Play?id=` alone plays from a queue entry; the
  queue id changes on every modification, including a delete (as API
  §2.1/§5.1 say; azzurro's note was wrong).

Releases (Misja, 2026-11-08):
- Semantic versions, starting carefully at 0.4.0. `Version` in
  rpm/harbour-swell.spec and `VERSION` in harbour-swell.pro must match.
- A published release has `Release: 1` and an annotated tag `vX.Y.Z` on its
  merge commit. The tag subject becomes the RPM changelog entry
  (`sfdk-changelog --auto-add-annotations`). Test builds for the device bump
  `Release` instead.
- Published as a GitHub release with RPMs for aarch64 and armv7hl. armv7hl
  is built and tested in the SDK only; there is no armv7hl device.
- Build each architecture from a clean tree: the build runs in the source
  directory, and leftover objects from another architecture break
  `%install`.

Decisions and authorization scope (Misja, 2026-11-06):
- Name and licence: name Swell; MIT.
- Targets: build target 5.1.0.11-aarch64 mapped to release target
  5.2.0.18-aarch64.
- Sandbox: Sailjail OrganizationName io.github.misja, permission Internet;
  Audio is added by 003 for the lock-screen controls (Misja, 2026-11-08,
  Q2 in specs/003-mpris/spec.md, issue #13).
- Planning: 001–006 as in the spec. Add-by-address is in the MVP; MPRIS is
  003; preset zapping is "later".
- Branding: BluOS is mentioned descriptively only, with a trademark notice;
  no "(works with BluOS)".
- Languages: English is the source; Dutch is maintained; `qsTr` through
  `sailfishapp_i18n`.
- Approved:
  - the private GitHub repo;
  - pushes and PRs for swell and for sailfish-workspace#18 (both merged 2026-11-06);
  - the 002 PRs (#5, #6), merged on Misja's instruction 2026-11-06;
  - device installs through the SDK-registered device, which Misja confirms on
    the phone.
- Not approved: publication (Harbour, Chum, OBS).

Reviews (separate assistant agents; not a human audit):
- scope_review, design_review, scope_review_2, design_review_2: approve with
  changes, resolved;
- result_review (AC6): resolved and re-checked, "ready for maintainer review".
- 002: review_002 (scope/design), result review and its re-check, a design
  review of the queue-id adoption and two checks of its implementation; all
  findings resolved (details in specs/002-queue/evidence.md).

App icon: parked by Misja on 2026-11-06 (issue #3, on the board as Todo).
Three rounds of wave concepts were tried and the wave motif was rejected as a
whole. Learnings and options are in issue #3. The template placeholder stays.

Open items:
- Release-target ID: the device runs 5.2.0.22, the registry names 5.2.0.18.
  Runtime evidence on 5.2: 0.3.0-14 works on 5.2.0.22, including MPRIS
  (Misja, 2026-11-08; specs/003-mpris/evidence.md).
- After an icon change, the launcher keeps the old icon until lipstick
  restarts (`systemctl --user restart lipstick`, with Misja's approval);
  the installed PNGs were already correct (2026-11-07).
- `sfdk deploy --sdk` fails on the device (no D-Bus session bus over SSH). Use
  the deploy steps below.
- Connections to the player: 8 seen at once (expected from Qt pools). Watch
  over time.
- Feature 006 (player settings) is to be discussed with Misja.
- Input for 004 (Misja, 2026-11-06): the official app's main sections are
  Home, Favourites, Music, Players and Search. Consider them, especially
  Favourites, when designing browse/search.
- Accepted: the validator warning "binary not stripped".
- 002, not checked: the English plural with a locale that has no Swell
  translation.

Build and test: copy the tree to /home/ubuntu/swell-001 in the sailfish-sdk
container, then as ubuntu:
`sfdk -c target=SailfishOS-5.1.0.11-aarch64 build-shell sh -c "cd build/tests && qmake ../../tests/tests.pro && make && make check"`.
Build the app with `sfdk ... -c no-pull-build-requires build` and validate it
with `sfdk ... check -s harbour RPMS/*.rpm`. Bump `Release:` in
rpm/harbour-swell.spec for each device install.

Deploy: `sfdk -c target=… -c device="Xperia 10 II - Dual SIM (ARM 64bit)"
deploy --manual`, then over SSH with the SDK key
(~/SailfishOS/vmshare/ssh/private_keys/sdk) to defaultuser@192.168.1.44:
`DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$(id -u)/dbus/user_bus_socket sdk-deploy-rpm ~/RPMS/<rpm>`.
Misja confirms the install on the phone.
