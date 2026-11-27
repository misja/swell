# Plan — 009 queue order

2026-11-27, after the scope/design review (approve with changes, applied).
Two pull requests: shuffle and repeat first, reordering second.

## PR 1: shuffle and repeat

- `xml`: `Status.shuffle`, `Status.repeat` (missing or unknown: off).
- `Client`: properties `shuffle`, `repeat` with NOTIFY from the status;
  `setShuffle(bool)` and `setRepeat(int)` through `send()` (gate by path).
  The reply is checked for an error only; the list follows the pid change.
- `NowPlayingPage.qml`: a `Row` under the transport row with two Silica
  `Switch`es, as `MediaPlayerControlsPanel.qml` (Sailfish.Media 0.9.18)
  does; visible with the transport row and without `streamUrl`.
- Tests: parser, client (state sent, two taps, failure), qmlruntime
  (compiles, hidden on `streamUrl`); the double's `/Shuffle` and `/Repeat`
  as measured.

## PR 2: reordering

- `Client::moveQueueEntry(from, to, pid, identity)`: one move at a time
  under the flag removes use (`m_removing` → `m_editing`), property
  `queueEditing` with NOTIFY; read-back with `fetchQueueWindow(from, from)`,
  `/Move` with `fetch()` and `setHoldPid(true)`, then
  `QueueModel::moveEntry` and the existing `adoptQueueId(to)` and
  `finishRemoval()`.
- `QueueModel::moveEntry(from, to)`: `beginMoveRows`, generation +1,
  re-request the window in flight or wanted (as `removeEntry`), shift
  `m_current` with the row.
- Drag: a `DelegateModel` over the queue model, `items.move()` on screen,
  as on the Presets page. The source move after `moved` is a visual no-op
  (Qt 5.6.3 `QQmlListCompositor::listItemsMoved`); a qmlruntime test
  checks it first, before the page is built on it.
- Shared component (`qml/pages/DragReorder.qml`): hold and dwell timers,
  edge scrolling, the lifted copy, start/cancel/drop/put-back; used by
  `PresetsPage.qml` (unchanged behaviour) and `QueuePage.qml`. The
  differences (grid vs list, cell size, `contentX`) are parameters.
- `QueuePage.qml`: "Reorder" in the pull-down; rows keep their
  `RemorseItem` and the page knows if one is `pending`; the mode as in
  spec C.
- Tests: client cases (AC 3), the double's `/Move`, the compositor test,
  a reload cancelling a drag, a refusal putting the row back.

## Alternatives considered

- Context menu "Play next" / "Move to end" instead of a drag: standard
  Silica, no own code. The maintainer chose the drag.
- Option 2 of the draft (offsets on the delegates, no `DelegateModel`):
  the lifted delegate would be destroyed past the cache buffer during
  edge scrolling, and it animates twice. Dropped in review.
- A shared queue of edits for moves and removes: not needed, since no
  drag starts while an edit runs. Dropped in review.
- `IconButton`s with `highlighted` for shuffle/repeat: the platform's
  media controls use `Switch`es; Swell follows them.
- No read-back before `/Move`: a stale move would move the wrong entry
  silently. Kept, as for removes.
