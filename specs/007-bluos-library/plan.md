# Plan — 007 bluos library

Status: revised 2026-11-08 after the scope/design review. Misja chose
option 3 (2026-11-08); reviewer re-check of option 3 pending.

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
  signals: `presets()` returns `QList<Preset>`, and so on. There is no
  `Q_PROPERTY` under the same name, so nothing is shadowed.
- **`Swell::QmlPlayer : BluOS::Client`** in `src/` adds the `QVariantList`
  properties `presets` and `inputs`. `PlayerModel::createClient` changes from
  private to protected virtual; that is Qt's usual factory method, not a new
  mechanism. `Swell::QmlPlayerModel` overrides it and is what the app
  creates.
- **`Swell::QmlBrowseModel : BluOS::BrowseModel`** adds `letters` and
  `moreLinks`, and overrides `data()` for the `actions` role. It is
  registered under the existing QML name `BrowseModel`. It needs no factory,
  because QML creates it.
- **The QML pages keep the same names and keys.**
- **Cost:** two small subclasses in `src/` and one `virtual` in the library.
- **For it:** what the issue asked for. A desktop or command-line app sees
  only typed C++.

Decision (Misja, 2026-11-08): option 3. It is the result the issue and Misja's original
question ("a pure C/C++ implementation others can reuse") ask for, at a small
cost. The runtime QML test (scope item 4) covers the risk of a page losing a
key.

## Other changes

- **Typed pointers:** `Q_PROPERTY(BluOS::QueueModel *queue …)` and
  `Q_PROPERTY(BluOS::Client *current …)`.
- **Images:** the parser structs keep their raw `QString` image.
  - `Client::presets()` and `inputs()` fill a separate `imageUrl` (`QUrl`)
    field on copies, resolved with `resolveImage`. This is the same pattern
    as the models' rows.
  - `Preset::inputId()` stays a method. The QML-facing map gets it as a key.
- **A1:** `onCommandReply` uses `errorIn`.
- **A2:** when stopped, `fetch` queues the handler with
  `QTimer::singleShot(0, this, …)`.
- **A3:** `fetchBrowse(key, q, context, handler)` and
  `fetchQueueWindow(start, end, context, handler)`. The handler runs only
  while the context object exists; internal callers pass `this`.
  `BrowseModel`'s own `QPointer` guard can stay as it is.
- **C2:** header comments without QML or page references.
- **Style:** touched code follows the Qt Coding Style, including its
  exceptions. There is no bulk commit.

## Checks and reuse

- **Existing suites:** xml, client, model, queue, browse, lsdp and qml. They
  change only where they read maps or call the fetch functions.
- **New client tests:** A1, A2 and A3.
- **New runtime QML test** (`tests/qml`, with a fixture player). It
  instantiates the real `QmlPlayer`/`QmlBrowseModel`, or the library classes
  under option 0. It then evaluates the expressions the pages use, for every
  key listed above, and `.length`, `[i]` and `push`.
- **Build:** SDK build and Harbour validator.
- **Device check by Misja.**

## Review triggers

- A short re-check by the reviewer of the chosen option before building, if
  Misja picks option 3 (as the review asked).
- Anything that changes request policy or timing reopens the review.
