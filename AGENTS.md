# Agent instructions — Sailfish workspace

Baseline version: 1.4.0 (2026-11-09).

These are operational instructions for coding assistants. Contributors should
start with CONTRIBUTING.md. The human maintainer retains responsibility for
merge and release decisions.

For Swell: this file is the Sailfish workspace baseline
(https://github.com/misja/sailfish-workspace), copied as the workspace asks.
CONTRIBUTING.md describes how Swell is built, tested and changed. Swell has
no ADRs; its decisions are in specs/. STATUS.md is the maintainer's working
handoff. The rules on OBS, runtime isolation and crypto do not apply to
Swell.

- Read STATUS.md and the relevant accepted ADR before project work. Project artifacts are English; use the maintainer’s language for discussion.
- AI tools may propose changes and run authorized checks. The maintainer reviews changes before merge. Identify automated checks, assistant review and human review accurately; assistant agreement is not a human audit. AI tools cannot approve their own work, waive acceptance criteria or authorize deployment/publication.
- Work autonomously on routine reversible local work within approved scope. Software installation, remote repository creation/push, OBS writes/publication, device/system/destructive changes and security exceptions require explicit scoped human approval. Architecture approval does not grant these permissions.
- Repositories and everything published from them are public; local facts stay local. Name roles ("the maintainer"), never other people from our own process; names appear only in outside information reproduced as is (licences, cited sources). No real IP/MAC addresses, host, device, room or share names: use placeholders. Required identity stays only in copyright/licence notices, packaging metadata, git authorship and account-based URLs/IDs. Check before every push, PR, issue or release (principles 1.5.0, "Personal information").
- Never inspect, import, export or log real private keys, password stores, OTP seeds/URIs, passwords or recovery codes. Use synthetic fixtures and published test vectors only.
- Preserve Jolla packages, binaries and libraries. Private runtime isolation must include ELF loading, helper paths, RPM resolution, configuration, agent sockets and upgrade/removal.
- Keep one accountable owner per task. Role consultation does not transfer ownership. Investigate blockers and propose options; allow one clarification round-trip for the same blocker, then escalate to its decision owner.
- Use distinct agents only on explicit delegation authorization. Independent security/release verification is required for crypto/isolation/release-critical changes; Builder self-tests do not count as independent approval.
- Pin source/tool/template versions and cite dated evidence. Separate facts, supplied observations, assumptions and unresolved questions. No silent upstream updates or automatic Chum webhooks.
- Use one spec/plan/task set per feature. Record concise artifact handoffs in STATUS.md. Do not create speculative app/common-library repositories.
- Run checks appropriate to the change; record failures and limitations. Never label unexecuted tests as passed.
- Child repositories must contain this essential baseline themselves: a parent file outside their Git root is not assumed loaded. Copy changes with baseline version and reviewed diff.

## Platform mechanisms in code

Firm maintainer instruction, 2026-11-08; workspace principles 1.4.0
(https://github.com/misja/sailfish-workspace/blob/main/docs/development/principles.md,
"Platform mechanisms in code"). Code here is reviewed, maintained and
learned from by people.
- Before writing a component, model, loader or workaround, look up the
  standard mechanism in the SDK target itself (the `qmldir` of
  Sailfish.Silica, the target's Qt headers) and cite it.
- Prefer Silica components, Qt model classes, properties with NOTIFY
  signals, explicit parameters and reacting to the actual change. Avoid
  look-alike components, hidden or zero-height rows, binding tricks or
  counters, behaviour derived from string contents, and timers that guess.
- If the platform has no mechanism and other apps do without, Swell does
  without too. Otherwise state the gap and ask the maintainer before
  building an own solution.
- Name any remaining own solutions in the pull request, unprompted.

## Mandatory review gate (maintainer instruction, 2026-10-15)

Apply the staged-role workflow before design/build work. Record a bounded scope,
acceptance criteria, accountable owner and required reviewer in the existing
issue/handoff. Substantial architecture/scope decisions require a separate agent
review before implementation, explicitly checking simpler alternatives and
maintenance cost. Crypto, runtime/package isolation and release-critical changes
also require a separate review of the resulting changes before maintainer review.
The maintainer explicitly authorizes delegation for these required reviews.
Builder self-review, changing role labels, passing tests or human merge approval
do not substitute for the separate assessment. Resolve findings first; report an
unavailable required reviewer rather than silently proceeding. Stay autonomous
within the reviewed scope; do not add speculative infrastructure or duplicate
process artifacts. Separate assistant review is not a human security audit or
release approval. Shared role details:
https://github.com/misja/sailfish-workspace/blob/main/docs/development/agent-governance.md;
child repositories retain this gate even when cloned alone.

## Research upstream and platform practice before design

Firm maintainer instruction, 2026-10-17; applies to the primary assistant and
all delegated agents. Prefer the least intrusive solution built on maintained
upstream behavior and actual Sailfish/RPM facilities. Before introducing a new
requirement, restriction, dependency or custom mechanism, inspect relevant
upstream documentation/source and existing platform packaging. Verify behavior
in the actual version in use; macro names and familiar patterns are not evidence.
Report what already solves the problem and what concrete gap remains. Reviewers
must challenge invented requirements as well as proposed implementations.

Do not create a project-specific service framework, process coordinator or
replacement for existing facilities merely to preserve an unexamined earlier
choice. Departures need a concrete necessity/maintenance justification and
maintainer agreement before implementation. Normal users must not inspect or
kill processes for routine package operations. Record findings in the existing
issue/handoff; do not create a parallel research backlog. Carry this requirement
into every delegated task and retain it across sessions.
