# Evidence — 005 grouping

## Build and tests (2026-11-08)

Branch 005-grouping, SDK target SailfishOS-5.1.0.11-aarch64.

- **`make check`:** all ten suites pass: spike 5, lsdp 18, xml 30,
  client 29, model 26, qml 18 (GroupPage.qml compiles), qmlruntime 17,
  queue 28, browse 26, group 18. No compiler warnings.
- **New suite `tests/group`, against `FakeHousehold`** (three players with
  per-player etags):
  - roles from both sides, and a primary that names itself as master;
  - adding one player, and several one at a time (the second request is
    sent only after the first answer); a silent add failure is reported;
  - removing one, and several in the comma-separated form; removing the
    primary of three (the inferred call);
  - group volume (`tell_slaves=1&level=`) and the player's own volume both
    sent, neither replaced;
  - a secondary's own volume change seen with the `/SyncStatus` watch and
    not without it (the latent bug the review predicted, shown in the
    fixture);
  - the `/Volume` answer shown at once;
  - a lone player sends no `/SyncStatus` long poll;
  - the model: the current player follows its primary; choosing a member
    chooses its primary; the group page starts and watches the members and
    stops them after; join and leave through the model.
- The group suite passed 3 of 3 runs in a row.
- **The `tests/qmlruntime` group values** (role, secondaryIds, reconnecting,
  groupVolume, groupBusy; model roles groupRole, leaderId, memberNames,
  volume) reach JavaScript for a lone player.
- **RPM 0.4.0-2:** builds, and `sfdk check -s harbour` succeeds with no
  warnings. Dutch is complete; English plurals are filled in.

## Device-check household

`tests/demo/household.py` (Python 3 standard library only) was smoke-tested
on loopback with curl:
- grouping through `/AddSlave`;
- the master and slave elements on both sides;
- a secondary serving its primary's `/Status`;
- `tell_slaves`;
- a member's `/SyncStatus` long poll held for its full timeout while only
  its leader changed;
- `/RemoveSlave`.

## Not verified

These have no real grouped hardware behind them (one player, the NAD C 379):
- removing a primary;
- the new primary;
- what `/Volume` without `tell_slaves` does on a primary;
- whether `/Presets` is proxied by a secondary.

## Device check (2026-11-08)

Misja checked 0.4.0-3 on the Xperia 10 II (Sailfish OS 5.2.0.22). The
emulated household ran on the host, at 192.168.1.2 on ports 11010–11030;
the phone reached it, ufw was active, and it was stopped afterwards. He went
through the list given before the check:
- grouping from the Now Playing pulley;
- the role line and the one-row group in Players;
- group volume and a member's own volume;
- choosing a member chooses the leader;
- ungrouping;
- the NAD as a lone player.

He reported "klopt allemaal" (all correct), noting that the navigation
takes some getting used to.
Follow-up: "Add player…" became "Add player" (no ellipses in menu
labels, Misja). Built as 0.4.0-4; all ten suites pass; the validator
succeeds.

## Review findings

Scope/design review, 2026-11-08 (approve with changes): H1–H3, M1–M5 and
L1–L6 applied to spec and plan (spec.md, plan.md). L1 (pair halves, Hubs and
players in a fixed group not offered as able to join) is built after the
PR was opened, at Misja's request: the model keeps the LSDP class
(0x0006 pair half, 0x0008 Hub) and `/SyncStatus` zoneSlave; the role
`canJoin` combines them; test `pairHalvesHubsAndZonesCannotJoin`. All ten
suites pass (xml 34, group 24).

## After the result review (2026-11-08)

- **`make check`:** all ten suites pass: xml 33 (groupVolume, the AddSlave
  and Volume answers), group 23 (3 of 3 runs). The others are unchanged.
- **RPM 0.4.0-3:** the validator succeeds with no warnings; Dutch is
  complete.
- **Pulley item:** "Group", not "Group…" (Misja, 2026-11-08).

Result review, 2026-11-08 (approve with changes):

| ID | Finding | Disposition |
|---|---|---|
| H1 | `setCurrentId` could recurse through stale group states | One level only; a stopped client's group state is not trusted; both households refuse leaders and the adder's own primary; test `aStaleGroupLoopDoesNotRecurse`. |
| H2 | "Can join" empty or stale | While the group page is open, every other player watches `/SyncStatus` in a new sync-only mode; candidates are lone players only; test `theGroupPageKeepsCandidatesCurrent`. |
| M1 | Members polled more than planned; the watch retried without backoff | Sync-only mode (no `/Status`, `/Presets`, `/RadioBrowse` for them); availability from the watch with exponential backoff; plan.md updated. |
| M2 | Membership read from each row's own state | From the current player's `secondaryIds` through `memberIds`. |
| M3 | A secondary of an unknown leader treated as a leader | Group page explains it, offers "Add <primary>", hides join, leave and the group volume. |
| M4 | groupPage flag robustness | Set for Activating or Active, cleared otherwise and on destruction. The mini-player is not shown on the group page, so `openNowPlaying` needs no case. |
| M5 | The Python household lacked two behaviours | `--refuse=N` and `--self-master`. |
| M6 | Test gaps | Added: membership change elsewhere, a lone player's request paths and counts, reconnecting, the XML answers. Runtime QML for a grouped player is not added (the QML test has one fake player); device check. |
| L1 | Persistence of a followed leader | Misja (2026-11-08): following the leader, also after the group ends, is the intended behaviour. |
| L2 | A timing margin in a test | Delay 1000 ms, wait 300 ms. |
| L3 | Ordering between the two SyncStatus reads | Accepted; heals within a second. |
| L4 | Group volume with a fixed-volume primary | The slider is hidden unless a level is known. |
| L5 | Role with a hostname and a 127.* id | Accepted; recorded. |
| L6 | Stale comments | `isSecondary()` renamed `hasMaster()`; comments, plan and STATUS fixed. |
| L7 | UI lines not on the canvas | "With A, B" is G3; "In another player's group" only for the unknown-leader case. Context menus of hidden member rows: accepted (remove after ungrouping). |
| L8 | dataChanged for all rows | Accepted; no extra requests. |
| L9 | Style | New long lines are wrapped where they were added. |

## Option A (grouping in the player list): result review, 2026-11-08 — TO APPLY

Separate assistant review of branch group-in-players (approve with changes;
not a human audit; nothing run by the reviewer). Option A was decided live
by Misja (canvas "Grouping, round 2") and assessed by this review instead of
a separate design review. Findings, not yet applied:

- H1 Group mute sticks while Per player is closed: groupMuted() trusts
  stopped members. Fix: skip members whose client is not running; test with
  watchGroup false (mute, then unmute).
- M1 Unknown-primary case dropped (was GroupPage "Add <primary>"): add a
  context item "Add %1" on a Secondary row with empty leaderId (needs a
  primaryId role), or record it as a decision.
- M2 "Follow" offered while the current player is a secondary: require the
  current player online and not Secondary in canJoin; test.
- M3 Fixed-volume leader without groupVolume shows a 0 slider: show the
  group slider only if groupVolume >= 0 or the leader's volume is not fixed.
- M4 Docs out of date: spec/plan/tasks/evidence still describe the group
  page and option B (also 001 spec :89, :276); record suite counts for this
  branch, 0.4.0-10 build/validator, the 0.4.0-9 device check; "Not
  verified": group mute with tell_slaves, a secondary's own mute.
- L1 pollAll also while Players is Activating (flicker of member rows).
- L2 watchGroup only while Qt.application.active.
- L3 leave(id) without leader: fall back to current only if it lists id.
- L4 Per player rows: follow memberIds order; unknown members missing.
- L5 Fixed-volume member slider at 0: hide it or say "Fixed volume".
- L6 Member IconButton height Theme.itemSizeSmall; check icon-system-volume
  in 5.x themes (exists on 5.2.0.22: checked 2026-11-08); "Per player" in
  primary colour; consider ExpandingSection.
- L7 Follow %1 host fallback; playermodel.h:87 comment; MutedRole comment;
  a memberIds role instead of the comma trick.
- L8 scrollToBottom with many members: accepted.
- Tests to add: leave with explicit leaderId; groupMuted with watchGroup
  false; exact mute=1&tell_slaves=1 target; canJoin when current is a
  secondary; runtime QML keys muted/canJoin and players.groupMuted.
