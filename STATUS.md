# Session handoff

Updated: 2026-11-06 (end of the 001 session). Decision owner: Misja Hoebe (@misja).

Current owner / next owner / decision owner: root / root (next session) / Misja.

Objective and acceptance criteria: feature 001 (MVP) is done and merged
(PR #2 → main 69bd6b1; issue #1 closed).
- Spec: specs/001-core-controller/spec.md, criteria AC1–AC6.
- PR https://github.com/misja/swell/pull/2 (branch 001-core-controller,
  closes issue #1) awaits Misja's review and merge.
- Shared conventions PR https://github.com/misja/sailfish-workspace/pull/18
  (principles 1.1.0).
- The UI design canvas and decisions are linked in issue #1.

Artifacts and evidence:
- bluos/ is the protocol library (namespaces BluOS, Lsdp).
- src/ holds the app stub and QML type registration; qml/ the pages.
- tests/ holds spike, lsdp, xml, client, model and qml.
- translations/ holds English (source) and Dutch (complete).
- evidence/001/README.md holds the current results for HEAD (release
  0.1.0-7: all suites pass, validator succeeded, 1 accepted warning) and the
  history.
- Device: the Xperia 10 II (Sailfish OS 5.2.0.20) runs 0.1.0-7, against a NAD
  C 379.

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
  - pushes and PRs for swell and for sailfish-workspace#18;
  - device installs through the SDK-registered device, which Misja confirms on
    the phone.
- Not approved: publication (Harbour, Chum, OBS).

Reviews (separate assistant agents; not a human audit):
- scope_review, design_review, scope_review_2, design_review_2: approve with
  changes, resolved;
- result_review (AC6): resolved and re-checked, "ready for maintainer review".

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
- Accepted: the validator warning "binary not stripped".

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
