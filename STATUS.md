# Session handoff

Updated: 2026-11-06. Decision owner: Misja Hoebe (@misja).

Current owner / next owner / decision owner: root / root / Misja.
Objective and acceptance criteria: specs/001-core-controller/spec.md
(MVP: Players, Home with presets and inputs, Now Playing, docked panel, cover, landscape).
Changed artifacts and evidence paths: AGENTS.md (baseline 1.3.0 copy), LICENSE,
specs/001-core-controller/{spec,plan,tasks}.md, bluos/, tests/, evidence/001/.
Decisions and authorization scope: name Swell and location ~/code/sailfish/swell
chosen by Misja 2026-11-06. Silica/QML + thin Qt 5.6 C++ (Rust 1.75 in SDK
3.13.5 excludes azzurro's Rust code). Private GitHub repo misja/swell (approved);
no installation, device or publication.
Scope review 2026-11-06 (separate agent scope_review): approve with changes.
All 12 findings applied to the spec: presets/queue/MPRIS split into 002/003;
armv7hl dropped; C++/QML per-part justification moved to the plan; Harbour
validator facts verified by root against the target files; synthetic fixture
server defined; error-state criteria added. The PDF link from the review
served no document; Misja supplied the v1.7 URL (below).
Maintainer decisions 2026-11-06: build target 5.1.0.11-aarch64 maps to release
target 5.2.0.18-aarch64; license MIT (LICENSE added); Rust fake-player allowed
locally if limited/isolated, with a concrete proposal and explicit approval
before any toolchain installation. Normative protocol source: BluOS Custom
Integration API v1.7 (sha256 87fd68fc…, not committed: reproduction forbidden);
spec updated with its polling rule, class IDs, privacy and marks rules.
Misja 2026-11-06: LXC container for fake-player and other work approved;
"compete" clause judged not applicable (Swell fills a gap).
Design review 2026-11-06 (separate agent design_review): approve with changes,
no blockers; findings verified by root (Docker masquerade/FORWARD DROP, host
ip_forward=0, API timeout/Delete text) and applied to plan.md/spec.md:
- C++ reason restated as testability; single shared NAM;
- /SyncStatus policy for grouped players; poll timeout of 100 s;
- LSDP Delete, split and class-filter tests; /Artwork redirect test;
- qemu spike first, with an i486 fallback;
- Layer B (demo-player) made optional and non-gating, using the same QtTest
  binary instead of a separate CLI driver;
- container isolation (since superseded, see below);
- RPM %files and OrgName rules; About firmware item dropped.
Misja 2026-11-06: demo test (Layer B) approved; no Docker on the host (Docker
inside a container is fine). Layer B now runs swell-test on the existing br0
(LAN address, reachable through sailfish-sdk's existing NAT): no new bridge,
no sailfish-sdk NIC, no host change; demo-player without --announce. This
isolation change needs a focused separate review before Layer B runs.
GitHub: private repo misja/swell, issue #1 on the Sailfish board (In
Progress), branch 001-core-controller.
Progress 2026-11-06: scaffold builds for aarch64 and passes the Harbour
validator (1 open warning: binary not stripped); evidence/001/README.md.
Misja 2026-11-06: qt5-qttest-devel may be added to the SDK (it was already in
the build snapshot; nothing installed); Sailjail OrganizationName io.github.misja.
Spike passed on aarch64/qemu (evidence/001/spike-aarch64.log).
Progress 2026-11-06: bluos/ library done with tests passing on aarch64
(evidence/001). UI work was paused at Misja's request to agree the interface
first. Design canvas and decisions are linked in issue #1. MVP scope agreed:
Players, Home (presets + inputs), Now Playing attached to Home, docked
mini-player, cover, landscape. spec.md has been revised accordingly. The UI
draft is stashed ("ui-draft-before-mvp-scope"), not committed.
Scope review of the MVP revision (separate agent scope_review_2, 2026-11-06):
approve with changes, no blockers. root verified its target claims: the
MediaPlayerControlsPanel always shows a slider; the target has no qmllint and
no offscreen platform; Silica defaults pages to portrait. All findings are
applied to spec/plan/tasks: start state, background polling, display-only
progress, inputs via §11.1 with the URL not re-encoded, preset edge cases, a
plain DockedPanel, a reworded AC4, renumbered ACs, citation of the pulley
guideline.
Remaining blockers, options and recommendation: a separate design review of
the revised plan (MVP rows, UI structure, AC4 test) is due before UI
implementation.
Next action: design review; meanwhile MVP library work (presets/inputs) stays
within the reviewed library design.
