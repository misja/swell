# Plan — 007 bluos library

Status: revised 2026-11-08 after the scope/design review. Misja chose
option 3 (2026-11-08); re-check of option 3 applied.

## How QML reads typed values

The pages use these lists as JS arrays:
- `.length` and `[i]`: HomePage.qml for presets and inputs, LetterIndex.qml
  for letters (which also builds a new array with `push`);
- lists passed on into JS objects: BrowsePage.qml for actions and
  `moreLinks`.

Map keys: presets `id, name, image, inputId`; inputs `id, text, image, url`;
letters `text, row`; moreLinks `text, browseKey`; actions `type, text, url`.

Rejected after review (H1):
- `Q_GADGET` lists read straight by QML;
- converters registered in `src/`.

The Qt 5.6.3 `libQt5Qml` only uses `QSequentialIterable` in the list
accessor behind `Repeater { model: }`. `QV4::ExecutionEngine::fromVariant`
does not use it and does not apply registered converters. So `.length`,
`[i]` and `push` would very likely fail. This was inferred from the binary
and not tested; it is enough to not build on it.

### Option 0: typed accessors beside the current properties

- The library keeps the `QVariantList` properties for QML and adds typed C++
  accessors next to them: `presetList()`, `inputList()`, `letterList()`, …
- **Cost:** the least change; the QML stays untouched.
- **Against:** the conversion for QML stays in the library, which the issue
  wanted to move to `src/`. Other Qt apps get both forms.

### Option 3: typed library, QML shaping in `src/`

- **The library offers only typed accessors** with the existing NOTIFY
  signals: `presets()` returns `QList<Preset>`, `letters()` a typed list of
  letters, `moreLinks()` a typed list of links. There is no `Q_PROPERTY`
  under the same name, so nothing is shadowed.
- **Typed actions:** for `ActionsRole`, the base `data()` returns
  `QVariant::fromValue(QList<BluOS::BrowseAction>)`, still filtered by
  `knownAction`. `Q_DECLARE_METATYPE` for the struct and the list goes in
  `xml.h`; that is QtCore only. The favourites row (an invalid `QVariant`) and
  an unloaded row (an empty list) keep their results.
- **`Swell::QmlPlayer : BluOS::Client`** in `src/` adds the `QVariantList`
  properties `presets` and `inputs`.
  - Their getters have their own names (`qmlPresets()`, `qmlInputs()`), so
    they do not hide `Client::presets()` (re-check L2).
  - moc in Qt 5.6.3 refuses a NOTIFY signal declared in the base class
    (re-check H1; confirmed on the SDK target, see evidence.md). The subclass
    therefore declares its own signals and forwards the base signals to them
    in its constructor.
- **Creating the player:** `PlayerModel::createClient` stays private. It
  calls a protected virtual `newClient(nam, host, port, parent)`, whose
  default is `new Client(...)`, and then sets the timing and the connections
  as today (re-check M1). `Swell::QmlPlayerModel` overrides only
  `newClient`, and `src/harbour-swell.cpp` creates it. No client is created
  in the constructor, so the override is reached.
- **`Swell::QmlBrowseModel : BluOS::BrowseModel`** adds `letters` and
  `moreLinks`, with forwarded signals as for the player. It overrides
  `data()` for `ActionsRole` only: it calls the base `data()`, which keeps
  the lazy loading, and converts the typed list to maps. Every other role
  goes through unchanged. It is registered with `qmlRegisterType` under the
  existing QML name `BrowseModel`. It needs no factory, because QML creates
  it.
- **Registration:** `Player` stays registered as the uncreatable
  `BluOS::Client`. QML reads properties through the object's runtime
  meta-object, so it sees the subclass properties behind a
  `BluOS::Client *`. The runtime QML test checks this.
- **The QML pages keep the same names and keys.**
- **Cost:** two small subclasses plus a model subclass in `src/`, one
  protected hook and a metatype declaration in the library.
- **For it:** what the issue asked for. A desktop or command-line app sees
  only typed C++.

Decision (Misja, 2026-11-08): option 3. It is the result the issue and Misja's original
question ("a pure C/C++ implementation others can reuse") ask for, at a small
cost. The runtime QML test (scope item 4) covers the risk of a page losing a
key.

## Other changes

- **Typed pointers:** `Q_PROPERTY(BluOS::QueueModel *queue …)` and
  `Q_PROPERTY(BluOS::Client *current …)`, fully qualified.
  `Client::queueModel()` then duplicates `queue()` and goes; the queue tests
  call `queue()` (re-check L3).
- **Images:** the parser structs keep their raw `QString` image.
  - `Client::presets()` and `inputs()` fill a separate `imageUrl` (`QUrl`)
    field on copies, resolved with `resolveImage`. This is the same pattern
    as the models' rows.
  - `Preset::inputId()` stays a method. The QML-facing map gets it as a key.
- **A1:** `onCommandReply` uses `errorIn`.
- **A2:** when stopped, `fetch` queues the handler with
  `QTimer::singleShot(0, this, …)` (context form, Qt 5.4 and later).
- **A3:** `fetchBrowse(key, q, context, handler)` and
  `fetchQueueWindow(start, end, context, handler)`. The handler runs only
  while the context object exists; internal callers pass `this`.
  `BrowseModel`'s own `QPointer` guard can stay as it is.
- **C2:** header comments without QML or page references.
- **Style:** touched code follows the Qt Coding Style, including its
  exceptions. There is no bulk commit.

## Checks and reuse

- **Existing suites:** spike, xml, client, model, queue, browse, lsdp and qml. They
  change only where they read maps or call the fetch functions.
- **New client tests:** A1, A2 and A3.
- **New runtime QML test** (`tests/qmlruntime`, with the fixture player). It
  compiles the `src/` classes and creates `QmlPlayerModel`/`QmlPlayer` and
  `QmlBrowseModel` through the app's own registration (re-check M3). It
  evaluates the expressions the pages use, for every key listed above, and
  `.length`, `[i]` and `push`. It also covers:
  - `Repeater { model: _player.presets }` with `modelData.id`, `name`,
    `image` and `inputId`, and `modelData.url` for inputs;
  - `model.actions` on the favourites row and on an unloaded row;
  - `parentActions` passed on through `_open(row)` and read by
    `_shuffleUrl`;
  - change notification: a binding on `_player.presets.length` updates when
    the player sends new presets. This catches a wrong signal forward.
- **A2 test:** the handler is not called before `fetch` returns.
- **Build:** SDK build and Harbour validator.
- **Device check by Misja.**

## Review triggers

- The re-check of option 3 is done (approve with changes, 2026-11-08); its
  findings are applied above and listed in evidence.md.
- Anything that changes request policy or timing reopens the review.
