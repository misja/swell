# Feature specification — 005 grouping

Status: revised 2026-11-08 after the scope/design review (approve with
changes; findings applied). Q1 and Q2 decided by Misja. No implementation and
no UI yet; the UI is agreed on the design canvas before it is built.

## Owner and linked issue

- Accountable owner: root (primary assistant session).
- Decision owner: Misja Hoebe (@misja).
- Issue: https://github.com/misja/swell/issues/21. 001 lists 005 in its
  "Agreed structure for later features": grouping on its own page, opened
  from Now Playing.
- Review: a separate agent reviews scope and design before implementation,
  and the result before the PR.
- UI: design canvas page "Grouping (005)", boards G1–G3, agreed by Misja
  on 2026-11-08 (the canvas is linked in issue #1):
  - **G1:** "Group…" in the Now Playing pulley, with the leader's role
    under its name, worded as azzurro does ("Leading 2 players",
    "Reconnecting…").
  - **G2:** the group page: group volume on top, then the members with
    their own volume and a switch, then the players that can join.
    Players are added one at a time; there is no "Group all".
  - **G3:** a group is one player (option B): one row "Living Room + 2" in
    Players. Tapping it, or a member, makes the leader current.
  - Mini-player and cover unchanged: they show the current player, which is
    always a leader or a lone player (Misja, 2026-11-08).
- Scope agreed by Misja (2026-11-08): show groups, group and ungroup players,
  and group volume. Fixed groups are out of scope.

## Problem

BluOS players can play together in a group. Swell shows nothing about groups
and cannot make or break them, so a household with more than one player still
needs the official app for that.

## Facts (2026-11-08)

- **BluOS Custom Integration API v1.7** (sha256 87fd68fc…, as in spec 001):
  - **Roles (§8):** a group has one primary player and one or more secondary
    players. Requests that a secondary proxies to the primary "include"
    `/Status`, playback control, the queue and browsing. `/Volume` is not in
    that list, which fits a secondary keeping its own volume.
  - **Grouping (§8.1, §8.2):** sent to the primary.
    - `/AddSlave?slave=IP&port=P`, optionally with `&group=Name`.
    - For several players at once: `/AddSlave?slaves=IP1,IP2&ports=P1,P2`.
    - The answer is `<addSlave>` with the slaves that were added.
    - A player that could not be added is simply missing from that list,
      with HTTP 200 (pyblu 2.2.0 `player.py:289-290`).
  - **Ungrouping (§8.3, §8.4):** `/RemoveSlave?slave=IP&port=P` (or
    `slaves=…&ports=…`), sent to the primary. It answers with the primary's
    `<SyncStatus>`.
  - **Removing the primary:** the API says the others then form a new group
    (groups of three or more). It does not say how to address that request,
    which player becomes the new primary, or what happens to a group of two.
    Home Assistant sends `/RemoveSlave` with the primary's own address to
    the primary. **Inferred, not verified on hardware.**
  - **Group status (§2.2):** in `/SyncStatus`:
    - `group` is the group name;
    - a primary lists `<slave id="IP" port="P"/>`;
    - a secondary has `<master port="P" reconnecting="…">IP</master>`.
    - The table says `master` appears only on a secondary. Yet the §2.2
      example shows a primary with `<master>` naming itself and its slaves
      listed. A role must therefore not be read from `master` alone.
    - The recommended `/SyncStatus` long-poll timeout is 180 s.
    - A player may report its own id as 127.0.0.1 (azzurro `docs/protocol.md`).
  - **Group fields in `/Status` (§2.1):** the primary's carries `groupName`
    and `groupVolume`.
  - **Volume (§3):** `/Volume?level=…&tell_slaves=1` changes the whole group;
    with 0, only the player addressed.
    - The API does not say whether every member is then set to the same
      level or keeps its relative level. Nor does it say what a primary does
      with `level` when `tell_slaves` is omitted, which is what Swell sends
      today. **Not verified.**
    - The `/Volume` answer carries the new level; Swell does not use it yet.
  - **Polling (§2):** a secondary's `/Status` is a copy of the primary's, so
    "/SyncStatus long polling is then necessary to track the volume of each
    secondary player".
  - **Fixed groups** (zones, `zoneMaster`/`zoneSlave`) are out of the API's
    scope.
- **Players that cannot join:** LSDP classes for pair-slaves and hubs, and
  `zoneSlave` players.
- **Swell today:**
  - `xml.cpp` already parses `group`, `master` (host, port, reconnecting)
    and the slaves (as "host:port") from `/SyncStatus`.
  - A secondary keeps its own volume (test `secondaryUsesItsOwnSyncVolume`).
  - Nothing reaches the UI.
  - `/SyncStatus` is read again when `syncStat` in `/Status` changes; there
    is no `/SyncStatus` long poll.
  - A secondary's `/Status` (with its `syncStat`) is the primary's copy. So
    a volume change on a secondary made elsewhere is not seen today: a
    latent bug (inferred from the API, not observed).
  - `isSecondary()` is `!masterHost.isEmpty()`, which would also call the
    §2.2 example primary a secondary.
  - Failed `/SyncStatus` reads do not change availability.
  - Only the current player polls, unless `pollAll` is set.
- **Emulation (only one real player is available, the NAD C 379):**
  - azzurro (jzbz/azzurro 10cce29, MIT) models a household in its Rust
    `demo-player`: three players on 127.0.0.2–.4:11000 with shared state.
    `/AddSlave` and `/RemoveSlave` set the leader, `/SyncStatus` is derived
    from it, and a follower serves its leader's status. Swell does not use
    the Rust code (decision 001: no foreign toolchain).
  - Its limits:
    - one etag and syncStat shared by every player, so it cannot show
      per-player changes;
    - `/RemoveSlave` answers `<addSlave/>`;
    - no `tell_slaves`, and no form for several players at once.

    It is a model, not evidence of player behaviour.
  - The phone has Python 3.11 (python3-base, a Jolla system package); the
    host has Python 3.14.
  - Swell accepts loopback addresses in "Add a player by address".

## Scope

1. **Library (`bluos/`), typed, no QML:**
   - `Client` exposes its group:
     - **Role:** alone, primary or secondary. Secondary only when `master`
       is present and differs from the player's own id or address; primary
       when it has slaves.
     - The group name, the primary's id (when secondary), the secondaries'
       ids (when primary), and whether the primary is reconnecting.
     - Changes come with `syncStatusChanged`, which exists already.
   - **Group commands on a primary:**
     - add one or several secondaries (IP addresses only, as the API asks);
     - remove one or several.

     They are additive actions: paced, never merged. An `<addSlave>` answer
     missing a requested player is reported as a failure.
   - **Group volume** (`tell_slaves=1`) has its own gate key. A lone
     player's `setVolume` request stays exactly as today. The level in a
     `/Volume` answer is taken over at once.
   - **Keeping a secondary current:** a `/SyncStatus` long poll (with etag,
     timeout 180 s), used only while the group page is open, for every
     member. With option B the current player is never a secondary.

     Such members need no `/Status` poll of their own, since a secondary's
     is the primary's copy. A failed `/SyncStatus` read counts for
     availability. A lone player keeps exactly today's requests.
2. **Player model:**
   - Which players form a group. Member references (`<slave id>`,
     `<master>`) are matched to model ids first by IP:port, then by each
     client's own reported `id`, ignoring loopback. Members that still do
     not match are shown by address.
   - Grouping, ungrouping and leaving go to the primary. If the primary is
     not in the model, "leave" is not offered; adding the primary at its
     address is.
   - Pair-slave, hub and zone-secondary players are not offered as able to
     join.
   - QML shaping goes in `src/`, as in 007.
3. **App (`src/`, `qml/`):** the UI is agreed on the canvas before it is
   built.
   - **Settled already:** a group page from Now Playing (001).
   - **Candidates for the canvas:** a group indicator on Now Playing, Home,
     the player list, the mini player and the cover; group volume and
     per-player volume on the group page; a primary shown as reconnecting.
   - Group controls are disabled while a group command is pending.
4. **Tests:**
   - **Fixture:** one C++ household in the fake player, with:
     - several players with per-player etags and syncStat;
     - a secondary whose `/Status` is the primary's document;
     - `/SyncStatus` long-poll holding;
     - `/AddSlave` and `/RemoveSlave` in single and multiple form,
       including a silent failure;
     - `tell_slaves`.
   - Client, model and runtime QML tests run against it.
   - **Device check:** the Python household on the host's LAN address (Q2).
     The real NAD is checked as a lone player only.

## Out of scope

- Fixed groups (zones), as in the API.
- Naming groups: the player's default group name is used.
- Moving playback between players (`canMovePlayback`).
- Grouping the real NAD with emulated players.
- A temporary client for a primary that is not in the model.
- LSDP announcements from the emulated players: they are added by address.
- Running emulated players on the phone.

## Decisions (Misja, 2026-11-08)

- **Q1, Home follows the group** (made concrete by option B: the current
  player is always the leader).
  - When the current player is a secondary, Home and Now Playing show the
    group's primary: its playback, presets and inputs. Playing a preset or
    input acts on the group's source.
  - The secondary keeps its own volume.
  - Expected use: a household has a main player (for Misja, the NAD) that
    leads, and others join it.
  - Whether playing on a secondary would ungroup it is avoided by sending
    those commands to the primary. **Not verified on hardware.**
- **Q2, the emulated players run on the local network, not on the phone.**
  - The host (192.168.1.2) has Python 3.14 but no Qt or C++ compiler.
  - So the device-check household is a stdlib-only Python script on the
    host: three invented players on 192.168.1.2, ports 11010, 11020 and
    11030 (one IP with several ports, as on the NAD CI580). The phone adds
    them by "host:port".
  - The automated tests keep the C++ household. Both follow one written
    behaviour list (plan.md) to limit drift.
  - Running it opens three ports on the LAN while it runs: started by hand,
    in the foreground, with Misja's approval each time. The host's ufw
    state is checked then.

## Acceptance criteria

1. The library exposes a player's group role, the group name, the members'
   ids and whether the primary is reconnecting, typed. Grouping, ungrouping
   and group volume are sent as the API describes. Client tests against the
   household cover:
   - adding one or several secondaries, and a silent failure;
   - removing one;
   - a secondary leaving through its primary;
   - `tell_slaves`, kept apart from the player's own volume;
   - both role shapes: a primary with and without a `<master>` naming
     itself;
   - removing the primary of a group of three, **labelled as an inferred
     call**.
2. With the group page open, a change made elsewhere (membership, or a
   member's volume) shows within one long-poll answer. The same holds for
   the current player while it is a secondary. A test covers this against
   the household.
3. A lone player, the only case the real NAD can show, behaves exactly as
   before:
   - Now Playing, volume, the queue and browsing work as today;
   - the existing suites pass unchanged;
   - a test shows its request pattern is unchanged, with no `/SyncStatus`
     long poll.
4. The UI agreed on the canvas is built as agreed. Runtime QML tests read
   the group values the pages use.
5. The SDK build and the Harbour validator pass, for aarch64 and armv7hl.
6. A device check by Misja covers:
   - the emulated household on the LAN (Q2): grouping, ungrouping,
     volumes, and Home following the primary (Q1);
   - the real NAD as a lone player.

   Behaviour that only real grouped hardware can show is recorded as not
   verified.
7. A separate result review before the PR.
