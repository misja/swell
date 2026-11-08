# Feature specification — 005 grouping

Status: draft 2026-11-08, for the scope/design review. No implementation and
no UI yet; the group page is agreed on the design canvas before it is built.

## Owner and linked issue

- Accountable owner: root (primary assistant session).
- Decision owner: Misja Hoebe (@misja).
- Issue: opened once Misja approves the spec. 001 lists 005 in its "Agreed
  structure for later features": grouping on its own page, opened from Now
  Playing.
- Review: a separate agent reviews scope and design before implementation,
  and the result before the PR.
- Scope agreed by Misja (2026-11-08): show groups, group and ungroup players,
  and group volume. Fixed groups are out of scope.

## Problem

BluOS players can play together in a group. Swell shows nothing about groups
and cannot make or break them, so a household with more than one player still
needs the official app for that.

## Facts (2026-11-08)

- **BluOS Custom Integration API v1.7** (sha256 87fd68fc…, as in spec 001):
  - **Roles (§8):** a group has one primary player and one or more secondary
    players. Secondary players proxy `/Status`, playback control, the queue
    and browsing to the primary.
  - **Grouping (§8.1, §8.2):** sent to the primary.
    - `/AddSlave?slave=IP&port=P`, optionally with `&group=Name`.
    - For several players at once: `/AddSlave?slaves=IP1,IP2&ports=P1,P2`.
    - The answer is `<addSlave>` with the slaves added.
  - **Ungrouping (§8.3, §8.4):** `/RemoveSlave?slave=IP&port=P` (or
    `slaves=…&ports=…`), sent to the primary. It answers with the primary's
    `<SyncStatus>`. Removing the primary from a group of three or more
    leaves the others as a new group.
  - **Group status (§2.2):** in `/SyncStatus`:
    - `group` is the group name;
    - a primary lists `<slave id="IP" port="P"/>`;
    - a secondary has `<master port="P" reconnecting="…">IP</master>`.
  - **Group fields in `/Status` (§2.1):** the primary's carries `groupName`
    and `groupVolume`.
  - **Volume (§3):** `/Volume?level=…&tell_slaves=1` changes the whole group;
    with 0, only the player addressed.
  - **Polling (§2):** a secondary's `/Status` is a copy of the primary's, so
    "/SyncStatus long polling is then necessary to track the volume of each
    secondary player".
  - **Fixed groups** (zones, `zoneMaster`/`zoneSlave`) are out of the API's
    scope.
- **Swell today:**
  - `xml.cpp` already parses `group`, `master` (host, port, reconnecting)
    and the slaves (as "host:port") from `/SyncStatus`.
  - A secondary keeps its own volume (test `secondaryUsesItsOwnSyncVolume`).
  - Nothing reaches the UI.
  - `/SyncStatus` is read again when `syncStat` in `/Status` changes; there
    is no `/SyncStatus` long poll.
  - Only the current player polls, unless `pollAll` is set.
- **Emulation (only one real player is available, the NAD C 379):**
  - azzurro (jzbz/azzurro 10cce29, MIT) models a household in its Rust
    `demo-player`: three players on 127.0.0.2–.4:11000 with shared state.
    `/AddSlave` and `/RemoveSlave` set the leader, `/SyncStatus` is derived
    from it, and a follower serves its leader's status. Swell does not use
    the Rust code (decision 001: no foreign toolchain).
  - The phone has Python 3.11 (python3-base, a Jolla system package); the
    host has Python 3.14.
  - Swell accepts loopback addresses in "Add a player by address".

## Scope

1. **Library (`bluos/`), typed, no QML:**
   - `Client` exposes its group:
     - a role (alone, primary, secondary);
     - the group name;
     - the primary's id when it is a secondary;
     - the secondaries' ids when it is a primary.
   - Group commands on a primary: add one or several secondaries, and
     remove one or several.
   - Volume of the whole group (`tell_slaves=1`) beside the player's own
     volume.
   - Request policy as today: the one-second rule and merging per path and
     parameter. Group commands are additive and never merged.
2. **Player model:**
   - Which players form a group, from the primaries' and secondaries' views.
   - Grouping, ungrouping and leaving a group addressed to the right primary.
   - While the group page is open, the group's members poll, as `pollAll`
     does for the player list.
3. **App (`src/`, `qml/`):** the UI is agreed on the canvas before it is
   built. Proposed content:
   - a group page from Now Playing: the players that can join, the current
     group's members, add/remove, group volume and per-player volume;
   - the group shown where the player is shown: Now Playing, Home, the mini
     player, the cover, and the player list (for example "Living Room +
     Kitchen").
4. **Tests:**
   - **Fixture:** the C++ fake player gets a small household (several
     players with shared group state, modelled on azzurro's). Client, model
     and runtime QML tests run against it.
   - **Device check:** a stdlib-only Python household, `tests/demo/household.py`,
     with three invented players on 127.0.0.2–.4. It runs on the phone over
     ssh, with Misja's approval each time; nothing is installed. The real
     NAD is checked as a lone player only.

## Out of scope

- Fixed groups (zones), as in the API.
- Naming groups: the player's default group name is used.
- Moving playback between players (`canMovePlayback`).
- Grouping the real NAD with emulated players.
- LSDP announcements from the emulated players: they are added by address.

## Open questions

- **Q1:** when the current player becomes a secondary, Now Playing shows the
  group's playback (proxied by the player). Do Home's presets and inputs
  stay those of the secondary, or are they hidden? The API does not say
  whether `/Presets` is proxied. Proposed: show what the player answers;
  checked against the household as far as it can be, otherwise marked as
  not verified on hardware.
- **Q2:** device-check emulation in Python on the phone (a script, no build)
  or as a C++/Qt program built for aarch64 (sharing the fixture code).
  Proposed: Python. It is lighter and needs no build; the automated tests
  stay in C++.

## Acceptance criteria

1. The library exposes a player's group role, the group name, and the
   members' ids, typed. Grouping, ungrouping and group volume are sent as
   the API describes. Client tests against the household cover:
   - adding one or several secondaries;
   - removing one, including the primary of a group of three;
   - `tell_slaves`;
   - a secondary leaving through its primary.
2. Group changes made elsewhere (the official app, another Swell) appear in
   Swell without a restart.
3. A lone player, the only case the real NAD can show, behaves exactly as
   before: Now Playing, volume, the queue and browsing. The existing suites
   pass unchanged.
4. The UI agreed on the canvas is built as agreed. Runtime QML tests read
   the group values the pages use.
5. The SDK build and the Harbour validator pass, for aarch64 and armv7hl.
6. A device check by Misja covers:
   - grouping, ungrouping and volume with the emulated household on the
     phone;
   - the real NAD as a lone player.

   Behaviour that only real grouped hardware can show is recorded as not
   verified.
7. A separate result review before the PR.
