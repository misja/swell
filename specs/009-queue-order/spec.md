# Feature specification — 009 queue order: shuffle, repeat and moving entries

Status: 2026-11-27, scope/design review by a separate agent: approve with
changes, applied (evidence.md, Reviews). No code yet.

## Owner and linked issue

- Accountable owner: the primary assistant.
- Decision owner: the maintainer.
- Issue: misja/swell#58.
- Review: a separate agent reviews scope and design before implementation,
  and the result before the PR.

## Problem

Spec 002 left shuffle, repeat and reordering the queue for later. A user
still needs the official app to shuffle the queue, to repeat it or one
track, or to move a track.

## Facts

- **The published API** (v1.7):
  - §4.6 `/Shuffle?state=0|1`: "creates a new queue by shuffling the current
    queue. The original (not shuffled) queue is retained for restore when
    shuffle is disabled." `state=1` "has no effect if queue is already in
    shuffled state". Reply `<playlist modified length shuffle id>`.
  - §4.7 `/Repeat?state=0|1|2`: 0 repeats the queue, 1 the current track,
    2 is off. Reply `<playlist length id repeat>`.
  - §5.3 `/Move?new=destination&old=origin`. Reply `<moved>moved</moved>`.
  - §2.1 `/Status`: `shuffle` (0/1), `repeat` (0/1/2). With `streamUrl`
    present, "shuffle and repeat are not relevant and should be removed
    from any UI if possible".
- **Measured** on a NAD C 379, BluOS 4.18.26, 2026-11-27, with a throwaway
  8-track queue while track 0 played (`evidence.md`):
  - `/Move?new=5&old=1` answered `<moved>moved</moved>`; the entry at 1 was
    then at 5, the others shifted up. The queue id rose by one,
    `<playlist modified="1">`, and the `/Status` etag changed, so the long
    poll returns.
  - `/Move?new=99&old=0`: answered `moved`; nothing changed, same id and
    etag. `/Move?new=3&old=3`: answered `moved`; order unchanged, id +1.
  - `/Shuffle?state=1`: new id; the playing entry stayed at 0, the rest
    were shuffled; `/Status` `shuffle` 1. Again `state=1`: same id, same
    etag. `/Shuffle?state=0`: the original order back, new id.
  - `/Repeat?state=1`, `2`, `0`: the reply carries `repeat`; the queue id
    is unchanged, `/Status` `repeat` follows and its etag changes.
  - `/Add?…&playnow=-1&where=last` ("Add last") on an empty queue while
    an input was paused started playing the queue. Tests on the device
    therefore need the maintainer present.
  - Second run (20:30, the maintainer present, low volume): shuffle
    reorders only the entries after the current one; `song` follows the
    playing entry through every move; shuffle off restores the order from
    before the shuffle and drops moves made while shuffled; while an input
    plays, shuffle, repeat and move are accepted and change the id as
    usual, and `song` is not meaningful.
- **Spec 002** (measured 2026-11-06): the queue id changes on every
  modification. Swell treats an id change it did not cause as a new queue
  and reloads; for its own deletes it adopts the new id when the queue
  matches (`QueueModel::adopt`).
- **Silica** (sailfishsilica-qt5 1.2.154, 5.1.0.11 target): no component
  for reordering a list (`qmldir` and sources searched for drag and move).
  The Presets page's reordering (spec 008, G) is Swell's own code, for a
  grid.
- **The platform's media controls** (on the phone, SFOS 5.2.0.22,
  2026-11-27): `Sailfish.Media` `MediaPlayerControlsPanel.qml`
  (sailfish-components-media-qt5 0.9.18), used by the Media app, shows
  shuffle and repeat as two Silica `Switch`es with an icon,
  `automaticCheck: false`, `checked` from the state:
  `icon-m-shuffle`; `icon-m-repeat`, or `icon-m-repeat-single` for repeat
  one, checked unless repeat is off. The icons are in
  sailfish-content-graphics-default (z1.75 on this phone). `Switch` with
  `icon`, `checked` and `automaticCheck` is in the target's Silica
  (Switch.qml:42–47). `Sailfish.Media 1.0` is an allowed Harbour import
  (allowed_qmlimports.conf, 5.1.0.11 tooling); the panel itself is a
  docked panel and not used.
- **azzurro** (jzbz/azzurro 6852dc7, supporting only): `/Move` confirmed on
  a Powernode with `new` as destination.

## Decisions (the maintainer, 2026-11-27)

1. Shuffle and repeat are two buttons (Silica `Switch`es, as the platform's
   media controls) on Now Playing, below the transport
   row. Repeat cycles off → all → one.
2. Moving entries is by drag, as on the Presets page: "Reorder" in the
   Queue page's pull-down menu starts an edit mode.

## Scope

Two pull requests under this one spec: first A–B (shuffle and repeat),
then C–D (reordering).

A. **Status.** `Status` parses `shuffle` and `repeat`. `Client` exposes
   `shuffle` (bool) and `repeat` (enum Off/All/One) with NOTIFY. Unknown
   or missing values count as off.

B. **Shuffle and repeat on Now Playing.**
   - Two Silica `Switch`es with icons: in portrait a row below the
     transport row; in landscape at both ends of the transport row
     (shuffle left, repeat right), so the volume slider stays in view (the
     maintainer, 2026-11-27, after the device check). One `Component` each,
     placed by a `Loader`. Shown as the platform's media controls show
     them (Facts):
     `automaticCheck: false`, `checked` from the status; `icon-m-shuffle`;
     `icon-m-repeat`, `icon-m-repeat-single` for repeat one.
   - Visible under the transport row's condition (`_empty`: online and
     nothing to play, `hasContent`, `xml.h:56`), and without `streamUrl`
     (API §2.1). Enabled while online.
   - Shuffle sends `/Shuffle?state=` the opposite of the status; repeat
     sends the next state (2 → 0 → 1 → 2). Through the existing gate by
     path.
   - The buttons follow `/Status` only. Two taps before the status has
     changed send the same value twice; the state advances once (for
     shuffle the second is ignored by the player, measured).
   - Shuffle's new queue id reaches the queue as a pid change Swell did
     not cause, so the list reloads (an inactive page reloads when next
     shown). The reply is not passed to `resetTo`: with the same length it
     would keep the old rows under the new id (`queuemodel.cpp`,
     `setLength`).
   - A failure gives the existing failure notice.

C. **Reorder mode on the Queue page.**
   - "Reorder" in the pull-down menu, above "Clear queue". Enabled when
     the player is online, the queue has two entries or more, no remove
     remorse is pending and no queue edit is in flight. A row keeps the
     `RemorseItem` that `remorseAction` returns (ListItem.qml:69); the
     page binds to its `pending` (private/RemorseBase.qml:56).
   - Moving does not depend on `streamUrl`: the queue can be ordered
     while an input plays (measured).
   - In the mode: rows have no context menu and a tap does not play; the
     header says the mode is on and that a tap ends it, as on the Presets
     page. The mode ends on a tap, when the page is left, the player goes
     offline, or fewer than two entries remain.
   - A reload or reset of the queue (`QueueModel.reloaded`, or the model
     reset) cancels a drag in progress; the mode stays, as on the Presets
     page. `lengthChanged` is not the trigger (it also fires on `adopt`).
   - Only loaded rows can be lifted.
   - **Drag**, as on the Presets page, with a `DelegateModel` and
     `items.move()` for the order on screen: a press held 150 ms lifts the
     row; a copy is moved by Qt (`MouseArea` `drag.target` set once
     lifted, `drag.axis: Drag.YAxis`, bounded to the page; Qt 5.6
     activates it only if the press kept the grab, so `preventStealing`
     holds for the whole mode and the list scrolls only at its edges: the
     maintainer, 2026-11-27), the delegate is hidden and kept
     (`DelegateModel.inPersistedItems`) also when edge scrolling takes it
     past the cache buffer (SilicaListView.qml:66); held 150 ms over
     another place, the rows make room (`move`/`displaced` transitions);
     the lifted copy moves only up and down and is not scaled (the
     maintainer, 2026-11-27, after trying 0.7.0-5: a full-width row
     scaled 1.1 left the screen and could be moved sideways);
     near the top or bottom edge the list scrolls; releasing sends one
     move; releasing where it started sends nothing. A window reply
     during a drag changes data by source row (`dataChanged`) and does not
     disturb it.
   - The drag code is shared with the Presets page: one component for the
     hold and dwell timers, edge scrolling, the lifted copy and
     start/cancel/drop/put-back, used by both pages. Own code, named once
     in the PR.

D. **Moving.** `Client::moveQueueEntry(from, to, pid, identity)`, with
   `from` the row's `model.position`.
   - Refused, nothing sent: offline; `from == to`; either position out of
     range; the queue's id is not `pid`; a remove or move in flight or
     pending. One move at a time, under the same flag as removes
     (`m_removing` becomes `m_editing`); `queueEditing` is a property with
     NOTIFY for "Reorder" and the drag start.
   - Like a remove, it first reads the entry at `from` back and sends only
     if the player's id is the one recorded and the entry is the same.
     Otherwise "The queue has changed. Nothing was moved."; the row goes
     back, and the list follows the status (a reload on a pid change).
   - Then `/Move?new=to&old=from` through `fetch()`, not through the
     merging gate, with the pid held as for a delete.
   - `moved`: `QueueModel::moveEntry(from, to)` moves the row
     (`beginMoveRows(…, to > from ? to + 1 : to)`), bumps the generation,
     re-requests a window that was in flight or wanted, and shifts the
     current index as the row moves (the status corrects it). The source
     move is a visual no-op after `items.move()` (Qt 5.6.3
     `QQmlListCompositor::listItemsMoved`, qqmllistcompositor_p.h:252,
     move ids l.273; checked by a test, AC 6). Then the existing
     `adoptQueueId(to)`: adopt the new id if the length is unchanged and
     the entry at `to` is the moved one, otherwise reload. `moved` alone
     proves nothing (it is also the out-of-range reply); the read-back is
     the proof.
   - Anything else (no answer, HTTP error, `<error>`): the failure notice;
     the row goes back.

E. **MPRIS** keeps `hasShuffle` and `hasLoopStatus` false (spec 003): they
   lock after the first D-Bus read. Out of scope here.

## End states

| Situation | Sent | Shown |
| --- | --- | --- |
| Shuffle tapped, off | one `/Shuffle?state=1` | button on after the status; queue list reloads (new id) |
| Shuffle tapped, on | one `/Shuffle?state=0` | button off; original order, list reloads |
| Shuffle or repeat tapped twice before the status changed | the same value twice, the second after the gate interval | the state advances once |
| Repeat tapped | `/Repeat?state=` next value | the new state after the status |
| Shuffle/repeat changed by another controller | — | buttons follow the status |
| An input or stream plays | — | shuffle and repeat hidden |
| Nothing to play (stopped, no `title1`, no stream) | — | hidden with the transport row |
| Player offline | — | buttons disabled |
| Shuffle/repeat reply fails | one request | the failure notice; buttons keep the status |
| Repeat one | — | `icon-m-repeat-single`, checked |
| Reorder: dropped at another place | read-back, one `/Move?new=to&old=from` | the new order; the new id adopted, no reload |
| Reorder: dropped where it started | nothing | unchanged |
| Reorder: queue changed before the drop (id or entry) | the read-back only | "The queue has changed. Nothing was moved."; the row back; list follows the status |
| Reorder: `/Move` fails | read-back and move | the failure notice; the row back |
| Reorder: adopt read-back does not match (another controller meanwhile, e.g. a shuffle) | read-back, move, read-back | the list reloads |
| Reorder: the queue reloaded or reset during a drag | nothing | the drag ends, the mode stays; the new queue |
| Reorder: fewer than two entries left | nothing | the mode ends |
| Reorder: page left or offline, during a drag | nothing | the mode ends; the row back |
| Reorder: a window reply during a drag | — | rows fill in; the drag continues |
| Reorder: a quick swipe in the mode | nothing | nothing moves; the list scrolls only at its edges |
| Reorder: dragged to the edge | — | the list scrolls, also past the cache buffer; placeholder rows load |
| Reorder: held on a placeholder row | — | nothing lifts |
| Reorder: a drag while a move or remove is in flight | — | no drag starts |
| Reorder: the moved entry is the playing one | as any move | marker follows the row; `song` agrees (measured) |
| Reorder: an input plays, queue not empty | as any move | the new order (measured) |
| Reorder while shuffled, then shuffle off | the move; later `/Shuffle?state=0` | the order from before the shuffle; the move is gone (player behaviour) |
| "Reorder" with fewer than two entries, offline, a remove remorse pending or an edit in flight | — | disabled |
| The last of two remove remorses undone while the first runs | — | "Reorder" enabled; a drag starts only after that remove is sent and settled (`queueEditing`) |
| Rotation during a drag | nothing | the row back |

## Out of scope

- Saving the queue as a playlist, seeking, track actions (love/ban).
- Shuffle and repeat on the cover, the lock screen or MPRIS.
- Moving several entries at once, "play next" shortcuts.

## Acceptance criteria

1. Parser tests: `/Status` `shuffle` and `repeat`, missing and unknown
   values; the `/Shuffle` and `/Repeat` replies; `<moved>`.
2. Client tests against the double, shuffle and repeat: the right
   `state`; two taps before the status send the same value; a failure
   gives `commandFailed`.
3. Client tests, moving: exactly one `/Move` after a matching read-back;
   none after a mismatch, a stale id, offline, `from == to`, out of range,
   or while an edit is in flight or a removal pending; a successful move
   adopts the new id without a reload; an adopt mismatch reloads; a
   failure gives `commandFailed` and leaves the rows; after `moveEntry`
   a window in flight is discarded and requested again.
4. The double follows the measurement: id +1 on every move (also
   `new == old`) and on shuffle on/off; unchanged for repeat, for an
   out-of-range move and for shuffle on when already on; the playing
   entry and those before it stay on shuffle; shuffle off restores the
   order from before the shuffle; `song` follows the playing entry on a
   move. The shuffled order itself is the double's own. Invented: the
   failures.
5. QML tests: the pages compile; the buttons hide on `streamUrl`.
6. A qmlruntime test with a `DelegateModel` over a list model: after
   `items.move(1, 5)` and then the source model's move 1 → 5, the order on
   screen is unchanged and the delegate is the same object. DragReorder
   on a plain ListView: a drop requests the move and `putBack()` restores
   the order (forward and backward); `cancel()` restores it and requests
   nothing; `end()` leaves no drag and a later release sends nothing. The
   pages call `end()` on a reload or reset (`onReloaded`, `onModelReset`);
   that wiring and the hold and dwell timers are checked on the device.
7. `make check` and the Harbour validator pass (aarch64, armv7hl); Dutch
   complete.
8. Device check by the maintainer on the NAD (playback will start):
   shuffle on and off, the repeat cycle, the buttons hidden on an input;
   the reorder mode, a drag with hold and dwell, edge scrolling
   over unloaded rows, a tap ending the mode, a drag cancelled by
   rotation, the Presets page's reordering unchanged; in landscape the
   Switches at both ends of the transport row and the volume slider in
   view; in portrait their own row; a rotation while the queue plays.
9. A separate result review before each PR. Own code named in the PR.
