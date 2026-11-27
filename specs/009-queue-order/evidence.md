# Evidence — 009 queue order

## Measurements

NAD C 379, BluOS 4.18.26, 2026-11-27 20:21 (UTC+1), with curl. Before: queue
empty (id 22), paused on an input, shuffle 0, repeat 0. A throwaway 8-track
album from the player's library was added; afterwards the queue was cleared.

| Request | Reply | Queue id | Order / state | `/Status` |
| --- | --- | --- | --- | --- |
| `/Add?…&playnow=-1&where=last` | `<playlist length="8" id="23" count="8" …>` | 22 → 23 | 8 tracks | state play (playback started) |
| `/Move?new=5&old=1` | `<moved>moved</moved>` | 24, `modified="1"` | entry 1 now at 5 | etag changed |
| `/Move?new=1&old=5` | `moved` | 25 | original order | etag changed |
| `/Shuffle?state=1` | `<playlist length="8" id="26" modified="1" shuffle="1" repeat="0">` | 26 | playing entry stays at 0, rest shuffled | shuffle 1 |
| `/Shuffle?state=1` again | same as before | 26 | unchanged | etag unchanged |
| `/Shuffle?state=0` | `<playlist … id="27" shuffle="0">` | 27 | original order | shuffle 0 |
| `/Repeat?state=1`, `2`, `0` | `<playlist length="8" id="27" modified="1" shuffle="0" repeat="N">` | 27 | unchanged | repeat follows, etag changes |
| `/Move?new=99&old=0` | `moved` | 27 | unchanged | etag unchanged |
| `/Move?new=3&old=3` | `moved` | 28 | unchanged | etag changed |
| `/Clear` | `<playlist length="0" id="29" …>` | 29 | empty | state stop |

Second run, 2026-11-27 20:30–20:31, volume lowered to 8 and restored to 32,
the maintainer present. The same album added (id 30), playback started.

| Request | Queue id | Order / state | `song` |
| --- | --- | --- | --- |
| `/Play?id=3` | 30 | unchanged | 3 |
| `/Shuffle?state=1` while 3 plays | 31 | 0–3 unchanged, 4–7 shuffled | 3 |
| `/Shuffle?state=0` | 32 | original order | 3 |
| `/Move?new=6&old=3` (the playing entry) | 33 | entry 3 now at 6 | 6 |
| `/Move?new=7&old=0` (from before the playing entry to after it) | 34 | shifted | 5 |
| `/Move?new=0&old=7` (back) | 35 | shifted | 6 |
| `/Shuffle?state=1` with 6 playing, one entry after it | 36 | unchanged | 6 |
| `/Move?new=7&old=1` while shuffled | 37 | entry 1 now at 7 | 5 |
| `/Shuffle?state=0` | 38 | the order before the shuffle; the move made while shuffled is gone | 6 |
| `/Play?url=<input>` | 38 | unchanged; `streamUrl`, state stream | 0 |
| `/Shuffle?state=1` while the input plays | 39 | unchanged | 7 |
| `/Move?new=7&old=1` while the input plays | 40 | entry 1 now at 7 | 7 |
| `/Repeat?state=1`, `0` while the input plays | 40 | unchanged; `repeat` follows | 7 |
| `/Shuffle?state=0` while the input plays | 41 | the order before the shuffle | 7 |
| `/Clear`, `/Pause` | 42 | empty | 0 |

End: paused on the input, queue empty, shuffle 0, repeat 0, volume 32.

Summary:
- Shuffle reorders only the entries after the current one; the played ones
  and the current one keep their places.
- `song` follows the playing entry through every move.
- Shuffle off restores the order from before the shuffle; moves made while
  shuffled are dropped.
- While an input plays, shuffle, repeat and move are accepted and change
  the queue id as usual; `song` there is not meaningful (0, then 7 after a
  shuffle).

## Platform

- Phone (SFOS 5.2.0.22), 2026-11-27: `/usr/lib64/qt5/qml/Sailfish/Media/MediaPlayerControlsPanel.qml`
  (sailfish-components-media-qt5 0.9.18): shuffle and repeat as `Switch`
  with `icon.source`, `automaticCheck: false`; `icon-m-shuffle`,
  `icon-m-repeat`, `icon-m-repeat-single`
  (sailfish-content-graphics-default-z1.75 1.4.6).
- SDK target 5.1.0.11: Silica `Switch.qml:42–47` (`checked`, `icon`,
  `automaticCheck`); no list reordering in Silica's `qmldir`;
  `RemorseBase.qml:56` `pending`; `ListItem.qml:69` `remorseAction` returns
  the item; Qt 5.6.3 `qqmllistcompositor_p.h:252` `listItemsMoved`, l.273
  `MovedFlags`. `Sailfish.Media 1.0` in `allowed_qmlimports.conf`.

## Reviews

- 2026-11-27, scope/design, separate agent: approve with changes. Applied:
  drag with `DelegateModel` (option 1) and a compositor test first; the
  drag code shared with the Presets page; one move at a time under the
  removes' flag instead of a shared edit queue, with `queueEditing`; the
  remove remorse seen through its `pending`; `moveEntry` bumps the
  generation, re-requests windows and shifts the current index; end
  states corrected (two taps, refusal without reload, a reload cancels a
  drag and keeps the mode) and added; visibility not tied to
  `hasContent`; no `resetTo` for the shuffle reply; two PRs; acceptance
  criteria extended. After the review, the platform's `Switch`es replace
  the planned `IconButton`s (found on the phone).

## PR 1: checks (2026-11-27, builder, automated)

- At 10f051c, SDK 5.1.0.11 aarch64, `make check`: 11 suites, 0 failed
  (queue 35, xml 44, qmlruntime 21, qml 21, presets 44, firmware 47, …).
  New: `TstXml::statusShuffleAndRepeat` (6 rows), `shuffleAndRepeatReplies`,
  `moveReply`; `TstQueue::shuffleAndRepeatFollowTheStatus`,
  `shuffleReordersAfterTheCurrentEntryAndReloads`, `shuffleOffRestoresTheOrder`,
  `repeatKeepsTheQueue`, `twoTapsBeforeTheStatusSendTheSameValue`,
  `failedShuffleIsReported`; `TstQmlRuntime::shuffleAndRepeat`;
  `TstQml::appFileCompiles(NowPlayingPage.qml)`. armv7hl tests not run.
- RPMs (`no-fix-version`, 0.7.0-1): aarch64 6d9b0654…, armv7hl b5b2dab8…;
  no compiler warnings. Harbour validator: 18 checks PASSED on both. rpmlint
  reports two errors on both, also present on main (spec unchanged):
  `no-changelogname-tag` (a branch build; release builds take the tag) and
  `description-line-too-long` (`rpm/harbour-swell.spec` line 21).
- Not run on the device yet.
- Result review of PR 1 (10f051c), separate agent, 2026-11-27: approve
  with changes, no blocking findings. Applied in 8c903d8: a qmlruntime test
  calls `setRepeat(Player.RepeatQueue|RepeatTrack)` from QML and checks
  `/Repeat?state=0|1`; the two-taps test checks the spacing (≥ 150 ms
  arrival) and a single id change; spec B wording on visibility (hidden
  with the transport row); the measured `/Repeat` reply in full; one null
  check form in the new QML; landscape added to the device check. Not
  applied: renaming `streaming` (same name and meaning as
  `QueueModel.streaming`); a shared constant for "2 = off" (the xml layer
  does not know `Client`).
- At 8c903d8, aarch64: 11 suites, 0 failed (queue 35, qmlruntime 21, xml 44).
- Device check of 0.7.0-2 (8c903d8 with Release 2), 2026-11-27 21:12–21:24,
  the maintainer on the phone, the NAD at volume 8 with a throwaway 8-track
  queue; NAD status logged alongside. Seen (maintainer, five screenshots):
  the Switches as in the Media app; shuffle on/off reached the player
  (pid 44, 47–53), the queue order changed and came back, the playing entry
  kept its place (also while paused, song 1); the repeat cycle all → one →
  off → all (status 0 → 1 → 2 → 0), `icon-m-repeat-single` for one; the
  Switches hidden on an input and back on the queue. Landscape: the volume
  slider moved below the fold (screenshot 21:20:16); the maintainer chose
  the Switches beside the transport row in landscape. Seen in the log, not
  part of this feature: after shuffle off while paused on entry 1, play
  started at entry 0 (21:22:09 → 21:22:29). Afterwards: queue cleared (only
  the throwaway album), repeat 0, preset 2 playing, volume 32.
- Device check of 0.7.0-3 (25fde2d with Release 3), 2026-11-27 21:43, the
  maintainer: landscape screenshot shows shuffle left and repeat right of
  the transport row, the volume slider fully in view. At 25fde2d, aarch64:
  11 suites, 0 failed (qml: NowPlayingPage.qml compiles).
