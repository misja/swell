# Plan — 005 grouping

Status: revised 2026-11-08 after the scope/design review. Q1 and Q2
decided (spec.md).

## Library

- **Role:**
  - `SyncStatus` gains `isPrimary()` (has slaves).
  - `isSecondary()` becomes: `master` present and not this player's own id
    or address.
  - `Client` adds typed accessors:
    - `groupRole()` (Alone, Primary, Secondary);
    - `groupName()`;
    - `primaryId()` ("host:port");
    - `secondaryIds()`;
    - `primaryReconnecting()`.
  - Changes come with `syncStatusChanged`; there is no new signal.
  - `/Status` `groupVolume` is parsed for the group volume slider;
    `groupName` comes from `/SyncStatus`.
- **Commands on a primary:**
  - `addSecondaries(QList<QPair<QString,int>>)` sends `/AddSlave`: the
    single form for one, `slaves=…&ports=…` for several. IP addresses only.
  - The `<addSlave>` answer is compared with the request. Missing players
    give `commandFailed`.
  - `removeSecondaries(…)` sends `/RemoveSlave` the same way. Its answer is
    a `<SyncStatus>` and is taken over.
  - Both go through `fetch()`: paced, never merged, like `runAction`.
- **Volume:**
  - `setGroupVolume(level)` sends `/Volume?level=…&tell_slaves=1` under its
    own gate key. The gate key is path plus first parameter name, so it
    gets a distinct key rather than sharing `/Volume?level` with
    `setVolume`.
  - `setVolume` is unchanged.
  - The level in a `/Volume` answer updates the player's volume at once.
- **`/SyncStatus` long poll:**
  - `setSyncPolling(bool)` starts or stops an etag long poll with a 180 s
    timeout, under the one-second rule.
  - While it runs, a failed read counts for availability (backoff as for
    `/Status`).
  - The `/Status` poll is independent and stays as it is.

## Player model

- **Home follows the group (Q1):** while the current player is a secondary,
  Home's presets, inputs and play commands use the primary's client. The
  volume stays the secondary's own.
- **Polling policy** (`applyPolicy`):
  - The current player polls `/Status` as today. It also polls
    `/SyncStatus` while it is a secondary.
  - With `groupPage` set (a new property the group page sets), every other
    member polls `/SyncStatus` only.
  - `pollAll` is unchanged.
  - A lone current player gets exactly today's requests (AC3).
- **Matching:** `memberId(ref)` matches by host:port, then by the clients'
  reported `SyncStatus.id`, ignoring 127.0.0.1. Unmatched members are kept
  as an address.
- **Actions:**
  - `group(primaryId, ids)` and `ungroup(primaryId, ids)`;
  - `leave(id)` sends `/RemoveSlave` to the secondary's primary. It is
    refused when the primary is not in the model; the UI offers to add it
    by address.
  - "Can join" excludes pair-slave, hub and zone-secondary players.
- **QML shaping** in `src/` (`QmlPlayerModel`/`QmlPlayer`), as in 007.

## App

- **UI:** proposals on the design canvas first. The group page is settled;
  the indicators and volume layout are candidates (spec scope 3).
- **Pending commands:** group controls are disabled while a group command
  is pending.
- **Strings:** new strings in English and Dutch.

## Tests

- **One C++ household** in `tests/client/fakeplayer.h`:
  - `FakeHousehold` holds N FakePlayers with per-player etag and syncStat;
  - a secondary's `/Status` is the primary's document;
  - `/SyncStatus` long-poll holding (like `/Status` today);
  - `/AddSlave` and `/RemoveSlave` in single and multiple form, with a
    switch for a silent AddSlave failure;
  - `/Volume` with `tell_slaves`;
  - a switch for the §2.2 primary-with-own-master shape.
- **Behaviour list.** Both households follow this list:
  - per-player etag and syncStat;
  - a secondary's `/Status` is the primary's document;
  - `/SyncStatus` long-poll holding;
  - `/AddSlave` and `/RemoveSlave` in single and multiple form;
  - a silent AddSlave failure;
  - `tell_slaves`;
  - the own-master primary shape.
- **New tests** for the client, the model (matching, policy, request
  pattern for a lone player) and runtime QML.
- **Device check (Q2):** `tests/demo/household.py`, stdlib only.
  - Three invented players on one LAN address with ports 11010, 11020 and
    11030 (default 192.168.1.2, given as an argument).
  - Started by hand in the foreground; stopped with Ctrl-C.
  - Every name and MAC is invented (`02:…`).
  - It is not packaged.

## Risks

- Real players may differ from the API and the emulation: timing,
  reconnecting, the new primary after removing one, what a secondary's
  `/Presets` answers, and whether playing on a secondary ungroups it. The
  device check records what was not verified.
- Extra polling while the group page is open: one `/SyncStatus` long poll
  per member, which is lighter than `/Status` polling every member.
