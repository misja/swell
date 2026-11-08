# Evidence — 007 bluos library

## moc and a NOTIFY signal from the base class (2026-11-08)

Target SailfishOS-5.1.0.11-aarch64, Qt 5.6.3. A subclass declares
`Q_PROPERTY(QVariantList list READ list NOTIFY listChanged)`, where
`listChanged` is a signal of the base class. moc fails:

    spike.h:18: Error: NOTIFY signal 'listChanged' of property 'list' does not exist in class Derived.

With a signal of the subclass's own, connected to the base signal in the
constructor, moc accepts it. At runtime the property reports
`notify=shapedListChanged()`, and emitting the base signal reaches it once.
The test project was a scratch build and is not kept in the repository.

## Runtime QML test against the code before the change (2026-11-08)

`tests/qmlruntime` (Probe.qml reads what HomePage, BrowsePage and the real
LetterIndex read) was written first and run on the SDK target against the
library as it was: 10 passed, 0 failed. All nine suites passed with it.

Negative control: in the SDK copy only, the preset key `inputId` was renamed
to `inputID`. The test then failed in `presets()` with `…|` where `…|input1`
was expected; the copy was restored and the suites passed again.

Recorded behaviour the change must keep: the favourites row gives JavaScript
`undefined` for its actions; a row not read yet gives an empty list; LetterIndex
leaves `#` out.

## Review findings and what became of them

Scope/design review, 2026-11-08 (approve with changes):

| ID | Finding | Disposition |
|---|---|---|
| H1 | QML in Qt 5.6 does not turn typed lists into JS arrays | Options 1 and 2 dropped (plan.md). |
| H2 | No runtime QML test | Scope item 4, acceptance criterion 4. |
| M1 | Library-only build target is redundant | Dropped. |
| M2 | No bulk brace commit | Touched-code rule (spec C1). |
| M3 | Keep raw image strings in the parser structs | Separate `imageUrl` field (plan.md). |
| M4 | Callbacks without a context object | A3. |
| M5 | Option 3 can be smaller | Option 3 as in plan.md. |
| L1 | Acceptance criterion 1 wording; `ActionsRole` conflict | Criterion 1 rewritten. |
| L2 | LSDP coverage, unseeded `qrand` | Noted in spec; out of scope. |
| L3 | HTTP 200 with an `<error>` body | Handled; no change. |
| L4 | No pimpl or ABI work | Out of scope (spec). |
| L5 | Tests for A1 and A2 | Acceptance criterion 3. |

Re-check of option 3, 2026-11-08 (approve with changes):

| ID | Finding | Disposition |
|---|---|---|
| H1 | NOTIFY of a base-class signal fails in Qt 5.6 moc | Confirmed above; forwarded signals of the subclass's own (plan.md). |
| M1 | `createClient` cannot be overridden as written | Protected virtual `newClient` hook. |
| M2 | Typed actions in the base `data()` unspecified | `QList<BrowseAction>` metatype; favourites and unloaded rows unchanged. |
| M3 | Gaps in the test plan | Runtime QML test extended; A2 test (plan.md). |
| L1 | Earlier findings not traceable | This table. |
| L2 | `presets()` getter in the subclass hides the typed one | `qmlPresets()`, `qmlInputs()`. |
| L3 | `queueModel()` duplicates `queue()` | Dropped. |
| L4 | Stale handoff in STATUS.md | Updated. |
