# Evidence — 008 deleting a preset

Raw files (reads, the capture, screenshots) stay local; they hold the
player's address and the maintainer's stations.

## 2026-11-26, NAD C 379, firmware 4.18.26

Reads, the assistant, read-only:
- `/Presets`: `prid="0"`, ids 1–7. `/Status` `<prid>0</prid>`.
- `/Services`: `/SetPreset` is declared only in "Add preset" menu entries.
  The "Remove favourite" entries are `/DeleteFavourite` for TuneIn and
  Airable, not player presets.
- `/ui/presets`: each preset has a `/ui/ContextMenu?…&type=preset`. The
  footer has "Reorder Presets" (`/reorder-presets` → `/Presets/edit`), and
  `refreshOnStatusChange key="prid"`.
- `/ui/ContextMenu` for preset 7: Play (`/Preset?id=7`), Edit (a deep
  link), Delete (`player-link /SetPreset?id=7`, `refreshScreen`).

The maintainer, in the official app; the assistant read `/Presets` and
`/Status` every second:

| Time | Action | `/Presets` ids | `prid` |
| --- | --- | --- | --- |
| before 20:37 | add a station in slot 8 | 1–8 | 1 |
| 20:41:19 | delete slot 8 | 1–7 | 2 |
| 20:55:05 | add slot 7 (with "Overwrite Volume") | 1–7 | 4 |
| 20:55:33 | add slot 8 | 1–8 | 5 |
| 20:55:49 | delete slot 7 | 1–6, 8 | 6 |
| 20:56:02 | delete slot 8 | 1–6 | 7 |

Between 20:41 and 20:55 the maintainer deleted two presets without the
watcher (`prid` 3).

The capture covers 20:54 to 20:56. It was made with tcpdump on the phone,
filtered on the player's address, and decoded on the workstation. The
official app sent:
- `GET /SetPreset?volume=66&image=…&encoded_url=…&name=…&canShuffle=0&id=7&shuffle=0`
  → 200, `<presets prid="4">` with ids 1–7, id 7 with `volume="66"`;
- `GET /SetPreset?image=…&encoded_url=…&name=…&canShuffle=0&id=8&shuffle=0`
  → 200, `<presets prid="5">` with ids 1–8;
- `GET /SetPreset?id=7` → 200, `<presets prid="6">` with ids 1–6 and 8
  (gzip, decoded);
- `GET /SetPreset?id=8` → 200, `<presets prid="7">` with ids 1–6.

Before a delete the app read that preset's `/ui/ContextMenu`; after it, the
app read `/ui/presets` again. In the screenshots the add flow is: source,
name, preset number (it warns "will overwrite the current Preset 7"), then
volume. The maintainer reports no confirmation before a delete.

## Reviews

- Scope, a separate agent, 2026-11-26: request changes. Resolved in the
  issue and this spec:
  - the fixed route instead of reading `/ui/ContextMenu` (spec 004);
  - `fetch()` instead of the merging gate;
  - the remorse records the player, the id and `prid`;
  - a refusal on a `prid` change;
  - `GridItem` with a page `RemorsePopup`;
  - the list from the `prid` refetch;
  - the end-state table;
  - observations labelled as such.
- Result, a separate agent, 2026-11-26: approve with changes. Resolved:
  - Swell records the version of the list shown, not the status `prid`.
    `presetsCurrent` follows whether `/Status` reports a newer one;
  - Delete is disabled while a remorse runs or the list is not current;
  - a null player is guarded;
  - the grid's height is as the Flow's;
  - the double changes the list only for an id-only `/SetPreset`, and
    marks the rest invented;
  - the send-time check uses `SendLog`;
  - a timeout row, and a list that is not read yet;
  - the README, and no station names here;
  - the page states are left to the device check (spec).
