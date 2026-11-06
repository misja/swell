# Implementation plan — 002 queue

Status: draft 2026-11-06, for a combined scope/design review. Not approved for
implementation.

## Linked specification

specs/002-queue/spec.md; builds on 001, merged as 69bd6b1.

## Design

| Part | Where | Reason |
|---|---|---|
| `parsePlaylist` (window or status) | C++, `BluOS` xml | The pure-parser pattern from 001, with QtTest coverage of optional and unknown elements. |
| `Status.song` | C++ (`parseStatus`) | The current queue position (§2.1). |
| `QueueModel` (one per `Client`, exposed as `Client.queue`) | C++ `QAbstractListModel` | Lazy loading through the standard `canFetchMore()`/`fetchMore()`, which `SilicaListView` calls as it scrolls. Each window is a `/Playlist?start=&end=` request of 50 entries through the client's NAM. Roles: position, title, artist, album, duration, image (resolved by the client's `resolveImage`), current. `length`, `pid` and a loading flag are properties. A list model is used here, unlike presets and inputs, because the queue can hold thousands of entries. |
| Refresh policy | C++ (`Client` → `QueueModel`) | Reset and reload on a `pid` change, and after a successful remove or clear. The current-row role updates on `song` changes. The model loads only while the Queue page is shown (an `active` flag set from QML), so background polling stays as it was in 001. |
| Commands | C++ (`Client`) | `playQueueEntry(position)` sends `/Play?id=`, `removeQueueEntry(position)` sends `/Delete?id=`, `clearQueue()` sends `/Clear`. They go through the existing gate (keyed by path and parameter) and report failures through `commandFailed`. |
| Queue page | QML/Silica | `SilicaListView` on `player.queue`, attached to Now Playing with `pushAttached`. `ListItem` with a ContextMenu "Remove" and `remorseAction`. A `PullDownMenu` with "Clear queue" through `RemorsePopup`, a `ViewPlaceholder` for the empty queue, an info label while not playing from the queue, `positionViewAtIndex` to the current row on open, and `VerticalScrollDecorator`. Touch targets use `Theme.itemSizeMedium`, labels highlight when pressed, and the page has `allowedOrientations: Orientation.All`. |

Now Playing gets its attached page the same way Home does: `pushAttached` on
activation when no next page exists.

## Validation

- **xml tests:** `parsePlaylist` windows, status form, optional fields, unknown
  and malformed input, `Status.song`.
- **client/queue tests** against the in-process FakePlayer:
  - `fetchMore` requests `start`/`end` windows and stops at `length`;
  - a `pid` change resets and reloads;
  - remove and clear send `/Delete?id=` and `/Clear`, then reload;
  - `/Play?id=` is sent;
  - an `<error>` body is reported;
  - nothing loads while the model is inactive.
- **qml:** the compile test picks up the new page automatically.
- **Build, validator, Dutch translation complete;** logs go to evidence/002/.
- **Device check by Misja.**

## Review

- Now: a combined scope/design review by a separate agent, covering the spec
  and this plan.
- Later: a result review before the PR.

## Risks and open points

- `/Play?id=` alone is observed but not documented on its own (§4.1 shows
  `seek&id`). If the device check fails, use `/Play?seek=0&id=`.
- Reads can be slow while the player fetches covers (azzurro). Windows of 50
  and a loading indicator at the list end keep the UI responsive.
- Queue positions shift after a remove. Swell reloads instead of patching
  locally, so a stale position is never reused.
