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
- **Refresh:** a `pid` change in `/Status` that Swell did not cause (a new,
  changed or cleared queue) resets the list, and so does reopening the page.
  `/Clear` replies with the new empty queue.
- **Current entry:** `/Status` `song` is the current position, and `pid` the
  queue identity. With `streamUrl` present the queue is not the source (§2.1).
- **Removing safely:** the queue id changes on every modification, including
  a delete (§2.1, §5.1; observed on the NAD C 379 on 2026-11-06, where
  azzurro's note said otherwise). Positions shift after a delete. A remove
  therefore names the position, the queue (`pid`) the user saw and the row's
  identity (`songid` or `fn`, plus title).
  - Just before `/Delete`, Swell reads that one position back. It sends the
    delete only if the player's id is the current one, the user's id is that
    one or an id it had before Swell's own deletes, and the entry still
    matches. Otherwise it says the queue has changed and removes nothing.
  - After its own delete Swell drops only that row and reads the new id back
    (`/Delete` answers only `<deleted>`). It takes the new id only if the
    length and the entry at that position match its list; otherwise it reads
    the list again. A pid change from the status poll meanwhile is not treated
    as a new queue.
  - Removes run one at a time; a waiting remove moves up by one for each
    earlier successful remove above it. A row remorse sends the row's current
    index, or the recorded position once the row is gone.
  - A Silica row remorse also fires when its row disappears or the page is
    left (RemorseItem.qml in the target), so these checks are what prevent
    deleting the wrong track, including the same track queued twice.
- **Commands, under the existing per-path gate and the one-second rule:**
  - play from an entry: `/Play?id=<position>`. §4.1 documents `id` together
    with `seek`; `id` alone works on the NAD C 379 (device check 2026-11-06),
    so `seek=0&id=` is not needed.
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

- Device check 2026-11-06 (NAD C 379, a LocalMusic album, 14 tracks): rows,
  play from an entry, the input line and clear work. A delete changed the
  queue id from 3 to 4, which the 0.2.0-1 build answered with a full reload;
  the adoption step above replaces that. LocalMusic entries have `fn` but no
  `songid`.
- Device check of 0.2.0-4 (2026-11-06): one remove keeps the list in place,
  two quick removes both go through, the input line and dimmed marker work.
- Not checked: the plural with a locale that has no Swell translation.

- API v1.7 §5 and §4.1 were read on 2026-11-06.
- Assumption: `time` and `image` are present on most services (azzurro
  observation). Rows work without them.
