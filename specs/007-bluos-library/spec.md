# Feature specification — 007 bluos library

Status: draft 2026-11-08, for the scope/design review. No implementation yet.

## Owner and linked issue

- Accountable owner: root (primary assistant session).
- Decision owner: Misja Hoebe (@misja).
- Issue: https://github.com/misja/swell/issues/8.
- Review: a separate agent reviews scope and design before implementation,
  and the result before the PR. No UI change, so no canvas.

## Problem

`bluos/` is meant to be a reusable Qt Core/Network library for BluOS
players: other Qt applications (desktop, command line) should be able to use
it. Some of its interfaces are still shaped for QML. Misja also asked
(2026-11-08) to check whether the API interaction follows tried and tested
patterns, and whether the code follows C/C++ conventions. That check is
recorded below. It decides what is in scope.

## Check of the API interaction (2026-11-08)

Compared with the BluOS Custom Integration API v1.7, and with the client that
Home Assistant uses: pyblu 2.2.0 from PyPI, and Home Assistant's
`bluesound/coordinator.py` on branch `dev`, both read on 2026-11-08.

| Practice | Swell (`bluos/client.cpp`) | Reference | Verdict |
|---|---|---|---|
| Status updates | Long poll `/Status?etag=&timeout=100`. Deadline: poll timeout + 15 s. | API §2.1: 100 s recommended, at least 10 s. Home Assistant: `poll_timeout=120`, `timeout=125`. | Same pattern. |
| SyncStatus | Read again when `syncStat` in `/Status` changes. | API §2.2. Home Assistant long-polls `/SyncStatus` separately. | Documented alternative; one connection fewer. |
| Request rate | At most one request per resource per second; precise timers. | API §2: a "must". pyblu does not enforce it. | Stricter than the reference. |
| Offline | Exponential backoff, 1 s up to 30 s. Etag dropped after a failure. | Home Assistant waits a fixed time. | Fine. |
| Timeouts | Every request has a deadline and is aborted when it passes. Qt 5.6 has no transfer timeout. | pyblu: total timeout per request. | Same. |
| Player paths | Only paths relative to the player. Sent byte for byte; refused if Qt would rewrite them. | pyblu: "Request URL must be relative to the player". | Same, stricter. |
| Errors | Network error, HTTP 404 (no control API), the `<error>` document. | pyblu: unreachable / unexpected response / command / browse error. | Same split, but as signals and strings rather than types. |
| Commands | Merged per path and parameter, so the last volume wins. Additive actions and deletes are never merged. | Not in pyblu. | Fine. |
| Images | `followRedirects=1` on player images. Qt follows the 301 itself. | API v1.7, image attributes. | Fine. |

Conclusion: the API interaction follows the documented and proven patterns.
No change of approach is needed. Small findings:

- **A1:** `<error>` parsing exists twice: `Client::errorIn` and inline in
  `onCommandReply`.
- **A2:** `Client::fetch` calls its handler synchronously when the client is
  stopped. A caller that sends again from the handler re-enters `fetch`.
  Today it is harmless, but it is an easy trap for library users.

## Check of C/C++ conventions (2026-11-08)

The reference is the [Sailfish OS Coding Conventions](https://docs.sailfishos.org/Develop/Apps/Coding_Conventions/),
checked 2026-11-08. They say: the Qt Coding Style and Qt C++ Coding
Conventions for C++; Qt5 connect syntax; ranged-for; CamelCase namespaces.
"Braces in one-line conditionals" is a QML rule.

- **Followed:**
  - Qt5 connect syntax throughout.
  - Ranged-for.
  - Namespaces `BluOS` and `Lsdp`.
  - `override` and `nullptr`.
  - `explicit` on single-argument constructors.
  - Parent ownership.
  - `QPointer` for replies and models that may go away.
  - Every reply is deleted with `deleteLater`.
  - No compiler warnings in the SDK build. The flags are qmake's defaults;
    no extra warning flags are set.
- **C1:** brace style is mixed. The Qt Coding Style leaves out braces on
  one-line bodies. Older code does that; newer code (`queuemodel.cpp`, parts
  of `browsemodel.cpp` and `client.cpp`) uses them. Counted with a simple
  pattern: about 140 one-line bodies without braces and 170 with.
- **C2:** the header comment of `client.h` says "usable … from QML". Some
  header comments refer to app pages.

## Interfaces shaped for QML (the issue)

- `Client::presets` and `Client::inputs` are `QVariantList`s of maps. The
  typed `Preset` and `Input` structs exist in `xml.h`.
- `Client::queue` and `PlayerModel::current`/`client()` return `QObject *`
  instead of `QueueModel *` and `Client *`.
- `BrowseModel::letters` and `moreLinks`, and the `actions` role, are
  `QVariantList`s of maps. The issue said 004 code already followed the
  typed rule; it does not, for these three.
- Model `data()` returns `QVariant`. That is the Qt model API, not
  QML-specific, so it stays.

## Scope (proposed)

1. **Typed pointers:** `Client::queue` → `QueueModel *`;
   `PlayerModel::current` and `client()` → `Client *`. The types are already
   registered with QML, so the QML pages need no change.
2. **Typed values:** `Client` offers `QList<Preset>` and `QList<Input>`, with
   images already resolved. `BrowseModel` offers `QList<Letter>` and
   `QList<BrowseItem>` for `moreLinks`, plus typed actions. The conversion to
   what QML reads moves to `src/` (design in plan.md).
3. **Library-only build check:** a test target that builds and links
   `bluos/` with `QT = core network` only, and fails if QtQml or QtQuick is
   needed.
4. **Small fixes:** A1 (one `<error>` parser), A2 (a stopped client answers
   asynchronously), C2 (comments without QML or page references).
5. **C1:** make brace style consistent with the Qt Coding Style in `bluos/`
   and `src/`. Hand-edited and without behaviour change, in a separate
   commit, so the review sees only layout changes.

## Out of scope

- Dropping Qt: network, XML and event loop would need replacing, which gives
  Swell nothing (as in the issue).
- A separate library repository, package or install target. The workspace
  says "Do not create speculative app/common-library repositories". Reuse
  stays possible through `bluos/bluos.pri`.
- New API features, and changes to timing or request policy.
- A formatter (`.clang-format`) or extra compiler warning flags as
  enforcement: new tooling; to be proposed separately if wanted.

## Acceptance criteria

1. `bluos/` has no QML-specific types in its public interface:
   - no `QVariantList` or `QVariantMap` properties;
   - no `QObject *` where a concrete type is meant.

   Model `data()` and `roleNames()` stay.
2. The library-only check builds without QtQml and QtQuick.
3. All existing test suites pass. The tests use the typed accessors where
   they now read maps.
4. The QML compile test passes. A device check by Misja shows Home (presets,
   inputs), Now Playing, Queue and Browse (letters, artist pulley, context
   menus) unchanged.
5. A1, A2 and C2 are done. C1 is a layout-only commit.
6. A separate result review before the PR.
