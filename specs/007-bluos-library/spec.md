# Feature specification — 007 bluos library

Status: revised 2026-11-08 after the scope/design review (approve with
changes; findings applied below). Misja chose option 3 (plan.md) on
2026-11-08. No implementation yet.

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
- **A3 (review M4):** the public `fetchBrowse` and `fetchQueueWindow` take a
  `std::function` but no context object, so a handler can outlive what it
  captured. `BrowseModel` guards with `QPointer`; `QueueModel` is safe only
  because it is the client's child. Qt's own pattern is a context argument,
  as in `QTimer::singleShot(ms, context, functor)`.
- **LSDP (review L2):** discovery adds the random timing the API asks for,
  but `qrand()` is never seeded. Fixing it changes timing, so it is noted
  here and left out of scope.
- **Not needed:** connection reuse is already right, because all clients
  share one `QNetworkAccessManager`, which keeps HTTP/1.1 connections open;
  pyblu shares a session the same way. Neither pyblu nor Home Assistant adds
  jitter to HTTP retries.

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
- **C1:** brace style is mixed. The Qt Coding Style uses braces only when a
  body has more than one line, with exceptions: a wrapped condition, both
  branches when either is multi-line, and empty bodies. Older code follows
  it; newer code (`queuemodel.cpp`, parts of `browsemodel.cpp` and
  `client.cpp`) puts braces on one-line bodies too. Decision (review M2):
  no bulk layout commit. New and touched code follows the Qt Coding Style,
  including its exceptions.
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

## Scope (revised after review)

1. **Typed pointers:** `Client::queue` → `QueueModel *`;
   `PlayerModel::current` and `client()` → `Client *`. Typed pointer
   properties already work in Swell on Qt 5.6 (`BrowseModel::client`), and
   the types are registered with QML.
2. **Typed values:** `Client` offers `QList<Preset>` and `QList<Input>`;
   `BrowseModel` offers typed letters, `moreLinks` and actions. Where QML
   reads them depends on Misja's choice of option 0 or 3 (plan.md). The
   parser structs keep their raw `QString` images; resolved images are a
   separate field or come from `Client::resolveImage` (review M3).
3. **Small fixes:** A1 (one `<error>` parser), A2 (a stopped client answers
   on the next event loop turn), A3 (a context object on the public fetch
   functions), C2 (comments without QML or page references).
4. **A runtime QML test** (review H2): a test that loads the real data
   through a fixture player (`tests/client/fakeplayer.h`) and reads every
   key the pages use:
   - presets: id, name, image, inputId;
   - inputs: id, text, image, url;
   - letters: text, row;
   - moreLinks: text, browseKey;
   - actions: type, text, url.

   It also checks the JS uses (`.length`, `[i]`, passing a list on). Today
   only compilation is tested, so a key that turns `undefined` shows as a
   blank tile and nothing fails.

Dropped after review:
- a separate library-only build target (M1): the six non-QML test suites
  already build `bluos/` with `QT -= gui` and no QML;
- a bulk brace-style commit (M2).

## Out of scope

- Dropping Qt: network, XML and event loop would need replacing, which gives
  Swell nothing (as in the issue).
- A separate library repository, package or install target. The workspace
  says "Do not create speculative app/common-library repositories". Reuse
  stays possible through `bluos/bluos.pri`.
- New API features, and changes to timing or request policy.
- A formatter (`.clang-format`) or extra compiler warning flags as
  enforcement: new tooling; to be proposed separately if wanted.
- Pimpl or ABI stability: the library is used as source through the `.pri`.
- The LSDP seed (see above) and anything else that changes timing.

## Acceptance criteria

1. The library offers typed C++ access to everything it exposes:
   - no `QObject *` where a concrete type is meant;
   - typed accessors for presets, inputs, letters, `moreLinks` and actions.

   With option 3, the library has no lists of string-keyed maps in its
   interface. The `actions` role then returns them from the src/ subclass.
   With option 0 they remain beside the typed accessors. Model `data()` and
   `roleNames()` stay.
2. The non-QML test `.pro` files stay free of qml and quick.
3. All existing test suites pass, plus tests for A1 (a command `<error>`
   reply), A2 (no synchronous handler call when stopped) and A3 (a handler is
   dropped when its context goes away).
4. The runtime QML test passes, along with the compile test.
5. The SDK build and Harbour validator pass.
6. A device check by Misja shows these unchanged:
   - Home: presets and inputs;
   - Now Playing;
   - Queue;
   - Browse: letters, artist pulley, context menus.
7. A separate result review before the PR.
