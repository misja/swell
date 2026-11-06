# Feature specification — 002 queue

Status: draft 2026-11-06, for a combined scope/design review (see plan.md).
Not approved for implementation.

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
3. The playing entry is marked, and the list scrolls to it when the page opens.
4. **Play from an entry** by tapping it.
5. **Remove an entry**: long press gives Remove, which runs a remorse in the
   row before the request.
6. **Clear the queue**: one pulley item "Clear queue", with a remorse popup
   before the request.
7. **States**:
   - an empty queue shows a placeholder with a hint to Home;
   - while an input or stream plays, a line says so, and tapping an entry
     plays the queue again;
   - an offline player shows the player's availability.

Excluded and left for later: saving the queue as a playlist, reordering,
"play next" or "add" actions (they come from browsing, 004), shuffle and
repeat.

## Required behavior

- **Reading the queue:** `/Playlist?start=&end=` in windows, never the whole
  queue in one request (API v1.7 §5.1). Further windows load as the user
  scrolls. `length` is the whole queue; `id` equals `/Status` `pid`.
- **Refresh:** reload when `pid` changes in `/Status` (a new or cleared queue),
  and after Swell's own remove or clear, because the queue id does not change
  on a delete (azzurro observation).
- **Current entry:** `/Status` `song` is the current position. With `streamUrl`
  present, the queue is not the source (§2.1).
- **Commands, under the existing per-path gate and the one-second rule:**
  - play from an entry: `/Play?id=<position>`. This is the §4.1 `id`
    parameter, observed alone by azzurro; the device check confirms it.
  - remove: `/Delete?id=<position>` (§5.2);
  - clear: `/Clear` (§5.4).

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
   empty documents.
2. **Fixture tests:** windowed reads with `start`/`end`; reload on a `pid`
   change; reload after remove and after clear; `/Play?id=`, `/Delete?id=` and
   `/Clear` sent as expected; a failure is reported.
3. **QML compile test** covers the new page.
4. **RPM:** builds and passes the Harbour validator; the Dutch translation is
   complete.
5. **Device check by Misja on the real player:** view a queue, play from an
   entry, remove, clear, and the input/stream line.
6. **A separate result review** before the PR.

## Evidence and assumptions

- API v1.7 §5 and §4.1 were read on 2026-11-06.
- The player currently has an empty queue (`length` 0). Rows with real data
  are checked on the device once a queue is filled from the official app.
- Assumption: `time` and `image` are present on most services (azzurro
  observation). Rows work without them.
