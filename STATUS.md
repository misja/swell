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

Progress: all MVP pages are built:
- Players, Add player, Home (presets, inputs) and About;
- Now Playing attached to Home, with a landscape layout;
- the docked mini-player and the cover.
The tests pass and the RPM is validated (evidence/001). Nothing is checked on
a device yet.

Next action:
1. Device testing via `sfdk deploy --sdk`. Misja chose to register the device
   with the SDK (option 2); find how that is done without the IDE GUI, or guide
   Misja through the IDE.
2. Device checks (LSDP over Wi-Fi under Sailjail, gestures, landscape, real
   player) with Misja.
3. The separate result review (AC6).
4. App icon/logo (Misja asked; the template placeholders must be replaced).

The old UI drafts are in git stash ("ui-draft-*") as starting material only.

How to build and test: copy the tree to /home/ubuntu/swell-001 in the
sailfish-sdk container, then
`sudo -iu ubuntu sfdk -c target=SailfishOS-5.1.0.11-aarch64 build-shell sh -c "cd build/tests && qmake ../../tests/tests.pro && make && make check"`.
The app is built with `sfdk ... -c no-pull-build-requires build` and
validated with `sfdk ... check -s harbour RPMS/*.rpm`.
