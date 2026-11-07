# Plan — 007 bluos library

Status: draft 2026-11-08, for the scope/design review.

## How QML reads typed values

The question is how QML reads `QList<Preset>`, `QList<Input>`, letters,
`moreLinks` and actions once the library exposes them typed. Options, least
intrusive first:

1. **Q_GADGET values, no adapter.**
   - The structs get `Q_GADGET` and `Q_PROPERTY` members. They are QtCore,
     not QML.
   - The library exposes `Q_PROPERTY(QList<BluOS::Preset> presets …)`.
   - QML would turn such a sequence into a JS array of value-type wrappers,
     but only if Qt 5.6's engine converts registered sequential containers
     (`QSequentialIterable`) and wraps gadgets. **Unverified for Qt 5.6.**
     A spike in `tests/qml` on the 5.1.0.11 target decides this before
     anything else is built.
   - If it works, the QML pages keep `modelData.name` etc. and `src/` adds
     nothing.
2. **Converters registered in `src/`.**
   - `QMetaType::registerConverter<QList<Preset>, QVariantList>()` in
     `src/registration.cpp`, with the gadget-to-map conversion there.
   - This only helps if the 5.6 engine uses registered converters when it
     reads a property. The same spike checks it.
3. **A QML-facing subclass in `src/`.**
   - `Swell::QmlPlayer : BluOS::Client` adds the `QVariantList` properties.
   - `PlayerModel` gets a client factory, so the app can create the
     subclass.
   - This works for certain, but needs more code and a factory hook in the
     library.

Which one is used depends on the spike. Option 3 is the fallback. The spike's
result and the choice are recorded here before the build.

## Other changes

- **Typed pointers:** `Q_PROPERTY(BluOS::QueueModel *queue …)` and
  `Q_PROPERTY(BluOS::Client *current …)`.
  - QML already knows both types (`qmlRegisterUncreatableType`).
  - Forward declarations stay where they avoid include cycles.
- **Resolved images:**
  - `Client` returns `Preset` and `Input` copies whose `image` is already
    resolved against the player.
  - `Preset::image` and `Input::image` become `QUrl`.
  - The XML parsers keep returning the raw string, in a separate field, so
    the parser tests stay as they are.
- **A1:** `onCommandReply` uses `errorIn`.
- **A2:** when the client is stopped, `fetch` queues the handler with
  `QTimer::singleShot(0, this, …)` instead of calling it inline.
- **C1:** a separate commit with layout only. Check:
  `git diff -w --stat` shows no token changes apart from braces.
- **Library-only check:** `tests/library/library.pro` with `QT = core
  network`, `include(../../bluos/bluos.pri)` and a `main` that creates a
  `PlayerModel` and a `BrowseModel`. If anything pulls in QtQml, it does not
  link.

## Checks and reuse

- **Existing suites:** xml, client, model, queue, browse and qml stay. They
  change only where they read maps.
- **Spike:** a QML test that reads `presets[0].name` and `letters[0].row`
  from a fixture client, on the target.
- **Build:** SDK build and Harbour validator.
- **Device check:**
  - Home: presets and inputs;
  - Now Playing;
  - Queue;
  - Browse: letters, the artist pulley, context menus.

## Review triggers

- The spike result is option 3. That is a larger change and needs a short
  re-check by the reviewer before building.
- Anything that changes the request policy or timing is out of scope; it
  would reopen the review.
