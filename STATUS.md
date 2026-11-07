# Session handoff

Updated: 2026-11-06 (feature 002, queue). Decision owner: Misja Hoebe (@misja).

Current owner / next owner / decision owner: root / root (next session) / Misja.

Objective and acceptance criteria: feature 004 (browse, issue #7) on branch
004-browse: specs/004-browse/{spec,plan,tasks}.md. UI agreed on the canvas
(B1–B9), scope/design and result reviews done and their findings fixed (also
the re-check of the device-feedback changes). Device: 0.3.0-6 installed.
Misja's walk-through (2026-11-07) led to decision 12 (navigation: play
returns to Home, mini-player on Browse/Search; canvas N1–N5) and no `#` in
the letter index, built as 0.3.0-7: result review, then install on Misja's
approval. Then: final device check, PR. Decisions: browse reads unpaced; no recently played
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
- evidence/001/README.md holds the current results for HEAD (release
  0.1.0-7: all suites pass, validator succeeded, 1 accepted warning) and the
  history. evidence/002/README.md holds the 002 results (0.2.0-4), reviews
  and device checks.
- Device: the Xperia 10 II (Sailfish OS 5.2.0.20) runs 0.3.0-6, against a NAD
  C 379.
- Device facts (2026-11-06): `/Play?id=` alone plays from a queue entry; the
  queue id changes on every modification, including a delete (as API
  §2.1/§5.1 say; azzurro's note was wrong).

Decisions and authorization scope (Misja, 2026-11-06):
- Name and licence: name Swell; MIT.
- Targets: build target 5.1.0.11-aarch64 mapped to release target
  5.2.0.18-aarch64.
- Sandbox: Sailjail OrganizationName io.github.misja, permission Internet only.
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
  findings resolved (details in evidence/002).

App icon: parked by Misja on 2026-11-06 (issue #3, on the board as Todo).
Three rounds of wave concepts were tried and the wave motif was rejected as a
whole. Learnings and options are in issue #3. The template placeholder stays.

Open items:
- Release-target ID: the device runs 5.2.0.20, the registry names 5.2.0.18.
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
