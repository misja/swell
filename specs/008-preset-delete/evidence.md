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

## 2026-11-26, adding (NAD C 379, 4.18.26)

Reads by the assistant, read-only:
- `/Browse?key=RadioParadise%3A&withContextMenuItems=1`: each station
  carries a context menu item `type="preset"`, "Add preset", with
  `actionURL="/SetPreset?image=…&name=…&service=RadioParadise&url=…"`. It has
  no `id`.
- A TuneIn station list has the same item on each station (177 in one
  list).
- Not seen on the second level of Airable, which is a menu, or on the
  capture inputs (`/RadioBrowse?service=Capture`).
- API v1.7 §7.1 documents `actionURL` ("may be invoked directly") but not
  the type `preset`.
- `/Services` declares "Add preset" as `/SetPreset` with `name`, `url` and
  an optional `image`. One form takes them from browse items (`text`,
  `URL`), one from now playing (`title1`, `streamUrl`).

Writes by the assistant, with the maintainer's approval, on throwaway
Radio Paradise stations. The maintainer's presets 1–9 were not touched.
Each request was sent as the player's `actionURL`, or as `/SetPreset?id=N`:

| Request | Reply | Presets after | `prid` |
| --- | --- | --- | --- |
| add The Main Mix (no `id`) | 200, the whole list | 1–10 | 13 |
| add Mellow Mix (no `id`) | 200, the whole list | 1–11 | 14 |
| `/SetPreset?id=10` | 200, the whole list | 1–9, 11 | 15 |
| add RockIt! (no `id`) | 200, the whole list | 1–11 (RockIt! in 10) | 16 |
| add Mellow Mix again | 200, the whole list | 1–12 (11 and 12 both Mellow Mix) | 17 |
| delete 12, 11, 10 | 200 each | 1–9, as before | 18–20 |

So without an `id`, the player takes the first free slot, and a gap is
filled. It does not refuse a station that is already a preset. `/Status`
reported every `prid`.

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
