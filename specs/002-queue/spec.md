# Feature specification — 002 queue

Status: revised 2026-11-06 after the combined scope/design review (review_002,
approve with changes; all findings applied). Implementation may start.

## Owner and linked issue

- Accountable owner: root (primary assistant session).
- Decision owner: Misja Hoebe (@misja).
- Issue: https://github.com/misja/swell/issues/4.
- UI: design canvas page "Queue (002)", agreed by Misja on 2026-11-06.
- Review: a separate agent reviews scope and design together before
  implementation, and the result before the PR.

## Problem and intended users

After 001, a user can start presets and inputs and control playback, but
cannot see what comes next or tidy up the queue that the official app,
Spotify Connect or another controller has filled.

## Scope and exclusions

Agreed with Misja on 2026-11-06:
1. A **Queue page** attached to Now Playing (swipe left; swipe right returns).
   The header is "Queue", with "<n> tracks · <player>" as description.
2. Rows show artwork, title, "artist · album" and the duration when the player
   gives one. The duration may be dropped after the device check (Misja: "see
   how it works").
3. The playing entry is marked, and the list scrolls to it when the page opens,
   even in a long queue. While an input or stream plays, the marker is dimmed,
   because `song` then means where playback resumes.
4. **Play from an entry** by tapping it.
5. **Remove an entry**: long press gives Remove, which runs a remorse in the
   row before the request.
6. **Clear the queue**: one pulley item "Clear queue", with a remorse popup
   before the request.
7. **States**:
   - an empty queue shows a placeholder with a hint to Home, and the pulley has
     no Clear;
   - while an input or stream plays, a line says so, and tapping an entry
     plays the queue again;
   - an offline player shows the player's availability.

Excluded and left for later: saving the queue as a playlist, reordering,
"play next" or "add" actions (they come from browsing, 004), shuffle and
repeat.

## Required behavior

- **Reading the queue:** `/Playlist?start=&end=` in windows of 50, never the
  whole queue in one request (API v1.7 §5.1). `length` is the whole queue, and
  `id` equals `/Status` `pid`.
  - The list has the full length from the start. Rows not yet loaded are
    placeholders, and the window the list shows is loaded. The window around
    the current entry loads first.
  - A window reply with a different `id` is ignored. A different `length`
    resets the list.
  - Reads get a longer deadline, because the player answers only after it has
    served cover art (azzurro observation).
  - A failed read leaves the rows as placeholders. They are retried when shown
    again, and no error is reported.
- **Refresh:** a `pid` change in `/Status` (a new or cleared queue) resets the
  list, and so does reopening the page. `/Clear` replies with the new empty
  queue. After Swell's own remove, only that row is dropped, so the view keeps
  its place.
- **Current entry:** `/Status` `song` is the current position, and `pid` the
  queue identity. With `streamUrl` present the queue is not the source (§2.1).
- **Removing safely:** positions shift after a delete, and the queue id does
  not change on one (azzurro observation; §5.1 does not say). A remove
  therefore names the position, the queue (`pid`) and the row's identity.
  Just before `/Delete`, Swell reads that one position back and sends the
  delete only if the queue and the entry still match. Otherwise it says that
  the queue has changed and removes nothing. Removes run one at a time; a
  waiting remove moves up by one for each earlier successful remove above it.
  A Silica row remorse also fires when its row disappears or the page is left
  (RemorseItem.qml in the target), so this check is what prevents deleting
  the wrong track, including the same track queued twice.
- **Commands, under the existing per-path gate and the one-second rule:**
  - play from an entry: `/Play?id=<position>`. §4.1 documents `id` together
    with `seek`, and `seek` is only valid when `/Status` has `totlen`. azzurro
    transcribed `id` alone from the official controller but never ran it. The
    device check tries `/Play?id=` and `/Play?seek=0&id=` while the queue plays
    and while an input plays, and the spec records the result.
  - remove: `/Delete?id=<position>` (§5.2). Removes are never merged by the
    gate; they run one at a time;
  - clear: `/Clear` (§5.4). Its reply resets the list.

  A failure is reported through the existing translated message.
- Unknown elements are ignored and malformed documents never crash.
  `<song>` children: `title`, `art`, `alb` (§5.1). `time` (seconds) and
  `image` are taken when present; they were observed but are not in §5.1.
- The Silica conventions and the UI quality rules from the workspace
  principles 1.1.0 apply. All strings are translated (English source, Dutch
  complete).

## Acceptance criteria

1. **Parser tests:** `/Playlist` windows, the status-only form (`length=1`),
   songs with and without `time`/`image`, unknown elements, malformed and
   empty documents; `Status.pid` and `Status.song`.
2. **Fixture tests:**
   - windowed reads that load the current window first;
   - a reset on a `pid` change and on a `length` change;
   - a late reply with an old `id` is ignored;
   - a failed read is retried;
   - a remove with a stale `pid` or row identity is refused, with no delete
     sent, also after another controller changed the queue;
   - two removes in quick succession run one after the other, also after
     leaving the page;
   - the same track queued twice: the chosen row is removed;
   - removing the current entry;
   - the `/Clear` reply resets the list;
   - `/Play?id=` is sent;
   - a command failure is reported;
   - nothing loads while the model is inactive, and a stale model reloads on
     activation.
3. **QML compile test** covers the new page.
4. **RPM:** builds and passes the Harbour validator; the Dutch translation is
   complete.
5. **Device check by Misja on the real player:** view a queue, play from an
   entry, remove, clear, and the input/stream line.
6. **A separate result review** before the PR.

## Evidence and assumptions

- Open until the device check: whether `/Delete` changes the queue id. If it
  does, a second remove queued before the first finished is refused (safe;
  the user repeats it).

- API v1.7 §5 and §4.1 were read on 2026-11-06.
- The player currently has an empty queue (`length` 0). Rows with real data
  are checked on the device once a queue is filled from the official app.
- Assumption: `time` and `image` are present on most services (azzurro
  observation). Rows work without them.
