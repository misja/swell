# Plan — 005 grouping

Status: draft 2026-11-08, for the scope/design review.

## Library

- **Group status:**
  - `SyncStatus` already holds group, master and slaves. `Client` adds typed
    accessors:
    - `groupRole()` (Alone, Primary, Secondary);
    - `groupName()`;
    - `primaryId()` ("host:port");
    - `secondaryIds()`.
  - It also adds a `groupChanged` signal, emitted when they change after a
    `/SyncStatus` read.
  - `/Status`'s `groupName` and `groupVolume` are parsed for the primary.
- **Commands on a primary:**
  - `addSecondaries(QList<host:port>)` sends `/AddSlave`: the single form
    for one, the `slaves=…&ports=…` form for several.
  - `removeSecondaries(QList<host:port>)` sends `/RemoveSlave` the same way.
  - These are additive actions: never merged, with errors reported through
    `commandFailed`.
  - Afterwards `/SyncStatus` is read again; the answer to `/RemoveSlave`
    already is one.
- **Volume:**
  - `setGroupVolume(level)` sends `/Volume?level=…&tell_slaves=1`, merged
    per path and parameter like today's volume.
  - `setVolume` stays the player's own.
- **Keeping members current:** a secondary's volume is only in its own
  `/SyncStatus`, and the group may change elsewhere.
  - Option A (proposed): while the group page is open, the model sets every
    member polling, as `pollAll` does. `syncStat` in each member's `/Status`
    then triggers its `/SyncStatus` read, as today. This adds no new request
    pattern.
  - Option B: a `/SyncStatus` long poll per member, as the API suggests in
    §2. It is a second long poll per player and a new pattern.

  The review decides. A stays within today's request policy.

## Player model

- `PlayerModel` resolves a group from its members' clients:
  - the primary's `secondaryIds`;
  - each secondary's `primaryId`.
- Players in a group but not in the model (not discovered, not added) are
  shown by address only.
- `group(id)`, `ungroup(id)` and `leave(id)` route to the primary's client.
  A secondary leaves with `/RemoveSlave` on its primary.
- The QML shaping of these values goes in `src/`, as in 007.

## App

- **UI:** proposals on the design canvas before anything is built:
  - the group page;
  - group indicators on Now Playing, Home, the mini player, the cover and
    the player list.
- **Strings:** new strings in English and Dutch.

## Tests

- **`tests/client/fakeplayer.h`:** a `FakeHousehold` with N FakePlayers
  sharing state.
  - `/AddSlave` and `/RemoveSlave`, single and multiple forms.
  - `/SyncStatus` with `slave`/`master` and a changing etag/syncStat.
  - A secondary's `/Status` is its primary's.
  - `/Volume` with `tell_slaves`.
  - It is modelled on azzurro's demo-player; no code is copied.
- **New tests** for the client, the model and runtime QML (AC1, AC2, AC4).
- **`tests/demo/household.py`:** stdlib only (`http.server`, threads), three
  invented players on 127.0.0.2–.4:11000. Every name, track and MAC is
  invented (MACs `02:…`). It is used only for the device check and is not
  packaged.

## Risks

- Real players may behave differently from the API and the emulation:
  timing, reconnecting secondaries, and what a secondary's `/Presets`
  answers. The device check records what could not be verified.
- Option A keeps several members polling while the page is open. That is
  the same load as the player list with `pollAll`.
