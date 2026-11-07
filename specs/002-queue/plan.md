# Implementation plan — 002 queue

Status: revised 2026-11-06 after the combined scope/design review (review_002,
approve with changes). All findings are applied below; implementation may
start.

## Linked specification

specs/002-queue/spec.md; builds on 001, merged as 69bd6b1.

## Design

| Part | Where | Reason |
|---|---|---|
| `Status.pid`, `Status.song` | C++ (`parseStatus`) | Queue identity and current position (§2.1). |
| `parsePlaylist` (window or status form) | C++, `BluOS` xml | The pure-parser pattern from 001. Per song: position, title, artist, album; `time`, `image`, `songid` and `fn` when present. |
| `QueueModel` (owned by `Client`, exposed as `Client.queue`) | C++ `QAbstractListModel` | Fixed length: `rowCount() = length`, so the scroll position and scroll-to-current work in long queues; not `fetchMore`. `data()` for an unloaded row returns empty roles and asks for its window of 50. At most one window request is in flight, plus the latest wanted window; earlier wanted windows are dropped. The window around `song` loads first. Each reply is checked: another `id` is ignored, another `length` resets. Reads use a 30 s deadline, because the player answers after serving covers. A failed window stays unloaded and is asked for again when shown. Roles: position, title, artist, album, duration, image (resolved), current, loaded. Properties: length, pid, loading, active. |
| Activity | QML → C++ | `active` = Queue page active and `Qt.application.active`, like PlayersPage. An inactive model loads nothing; its rows stay valid until the status poll reports another queue id, which marks it stale for a reload on activation. Loading starts while the page is swiped in (Activating). On a player change the old model is switched off. |
| Remove | C++ (`Client::removeQueueEntry(position, pid, identity)`) | Queued as (position, pid, identity), one at a time. Before `/Delete?id=`, a one-row `/Playlist` read confirms the current id (the user's id may be one from before Swell's own deletes) and the entry; on a mismatch nothing is sent and `removeRefused` is emitted. On success the model drops that row (`beginRemoveRows`), later queued removes above it shift by one, and a one-row read takes the new id if length and entry match (else a reload). While that runs, a status pid change does not reset the list. Removes do not go through the merging gate. Reviewed twice (result review, then a design review of the id adoption after the device showed that a delete changes the id). |
| Play from / clear | C++ (`Client`) | `playQueueEntry(position)` sends `/Play?id=` through the gate (works on the device). `clearQueue()` sends `/Clear`, and its `<playlist length="0" id=…>` reply resets the model. |
| Queue page | QML/Silica | `SilicaListView` on `players.current.queue` (null-safe; it follows a player change). It is attached to Now Playing with `pushAttached` on activation, as Home does. Rows are `ListItem`s at `Theme.itemSizeMedium`, with a thumbnail `sourceSize`, pressed highlight colours and a ContextMenu "Remove" through `remorseAction`. A `PullDownMenu` "Clear queue" (hidden when empty) uses `RemorsePopup.execute()`. Also: a `ViewPlaceholder` for an empty queue, an info label and a dimmed marker while not playing from the queue, `positionViewAtIndex(song)` once the current window has loaded, `VerticalScrollDecorator`, and `allowedOrientations: Orientation.All`. The header count uses `qsTr("%n tracks", "", length)` for plurals. |

## Validation

- **xml:** `parsePlaylist` (windows, status form, optional fields, unknown and
  malformed input), `Status.pid`, `Status.song`.
- **Queue fixture tests**, each listed in spec AC2. FakePlayer is extended to
  answer by query (a handler for `/Playlist` windows) and to hold or delay
  replies.
- **qml:** the compile test covers the new page.
- **Build, validator, Dutch complete** (including the plural form); results in
  specs/002-queue/evidence.md.
- **Device check by Misja:**
  - rows, durations and scrolling to the current entry;
  - play from an entry, trying `/Play?id=` and `seek=0&id=` while the queue
    plays and while an input plays;
  - remove, including two in a row;
  - clear;
  - the input/stream line.

## Review

- Done: the combined scope/design review (review_002); findings applied.
- Next: the result review before the PR. Reopen the design review if the
  model or remove design changes.

## Risks and open points

- Which form of play-from works is open until the device check.
- Slow reads while covers are served: a 30 s deadline, placeholders, and
  retry when a row is shown again. azzurro's cover gating is not copied
  (it would be speculative).
- A pending row remorse fires when its row is destroyed, for example on page
  pop, a reset or scrolling. The identity check turns a stale fire into a
  refused remove instead of a wrong delete.
