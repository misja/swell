# Session handoff

Updated: 2026-11-06. Decision owner: Misja Hoebe (@misja).

Current owner / next owner / decision owner: root / root / Misja.

Objective and acceptance criteria: specs/001-core-controller/spec.md, the MVP.
It covers Players (with add-by-address), Home with presets and inputs, Now
Playing attached to Home, a docked mini-player, the cover, landscape and About.
Criteria AC1–AC6. Issue: https://github.com/misja/swell/issues/1 (private repo
misja/swell, branch 001-core-controller). UI design canvas and decisions are
linked in the issue.

Changed artifacts and evidence paths:
- bluos/ is the protocol library: LSDP, XML parsers, Client, Discovery and
  PlayerModel.
- src/registration.* is the shared QML type registration.
- tests/ holds spike, lsdp, xml, client, model and qml.
- evidence/001/README.md has the dated results. The latest run on aarch64:
  all suites pass (5/18/19/22/23/4). The RPM passed the Harbour validator 1.110
  at scaffold time.

Decisions and authorization scope:
- Misja (2026-11-06):
  - name Swell; MIT;
  - build target 5.1.0.11-aarch64 mapped to release target 5.2.0.18-aarch64;
  - Sailjail OrganizationName io.github.misja, permission Internet only;
  - planning 001–006 as in the spec;
  - add-by-address stays in the MVP; MPRIS is feature 003;
  - Layer B (the azzurro demo-player) is removed.
- Approved: the private GitHub repo and qt5-qttest-devel in the SDK (it was
  already present).
- Not approved: device installation, publication, Harbour or Chum.

Reviews (separate agents, 2026-11-06), all "approve with changes" and resolved:
scope_review, design_review, scope_review_2 (MVP), design_review_2 (MVP plan).
The required result review (AC6) is still due after the UI.

Remaining blockers, options and recommendation: none. Open discussion:
player settings (006).

Progress: all MVP pages are built and checked on a device (Xperia 10 II,
Sailfish OS 5.2.0.20, with a real player). LSDP works under Sailjail and the
controls work; see evidence/001/README.md "Device check". The device is
registered with the SDK as "Xperia 10 II - Dual SIM (ARM 64bit)".

Deploy: in /home/ubuntu/swell-001, `sfdk -c device="Xperia 10 II - Dual SIM (ARM
64bit)" deploy --manual`, then on the device
`DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$(id -u)/dbus/user_bus_socket
sdk-deploy-rpm ~/RPMS/<rpm>`, and confirm on the phone. Plain `deploy --sdk`
fails there (no session bus over SSH).

Next action:
1. Small UI polish from the device check: vertical balance of Now Playing in
   portrait; watch the connection count over time.
2. The separate result review (AC6), then a PR for Misja.
3. App icon/logo, in a later session (Misja).
4. Release-target ID: the device runs 5.2.0.20, the registry says 5.2.0.18;
   to be settled with Misja.

The old UI drafts are in git stash ("ui-draft-*") as starting material only.

How to build and test: copy the tree to /home/ubuntu/swell-001 in the
sailfish-sdk container, then
`sudo -iu ubuntu sfdk -c target=SailfishOS-5.1.0.11-aarch64 build-shell sh -c "cd build/tests && qmake ../../tests/tests.pro && make && make check"`.
The app is built with `sfdk ... -c no-pull-build-requires build` and
validated with `sfdk ... check -s harbour RPMS/*.rpm`.
