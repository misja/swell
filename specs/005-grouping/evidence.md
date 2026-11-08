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

## Review findings

Scope/design review, 2026-11-08 (approve with changes): H1–H3, M1–M5 and
L1–L6 applied to spec and plan (spec.md, plan.md). L1 (pair-slave, hub and
zone players not offered as able to join) is not built: the model has no
LSDP class or zone flag per player yet; recorded for the result review.
