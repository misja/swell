# Plan — 006 firmware updates

Status: revised 2026-11-09 after the scope/design review; the spec's scope
and findings H1, M1–M7, L1–L5 apply. Open: the maintainer's approvals listed in tasks.md.

## Library (`bluos/`)

- **Parsing:** `xml.h` gets `UpgradeCheck { ok, available, inProgress,
  version }` with `parseUpgradeCheck`. `parseSyncStatus` recognises the
  `UpgradeStatusStage1/2` roots and reports the stage, instead of failing.
- **Client:**
  - `upgradeAvailable()` and `upgradeVersion()` with an `upgradeChanged`
    signal; `Availability::Updating` (appended) instead of a flag.
  - The check is read with the existing `fetch()` (paced) when the player
    comes online, and then on a timer of one hour while running.
  - `updating` is set:
    - by `inProgress="true"`;
    - by an `UpgradeStatusStage*` answer;
    - while a player that was updating does not answer.

    It is cleared only when `/SyncStatus` answers with a `SyncStatus` root, and,
    if it came from `inProgress`, a check no longer says so (result review H-A).
  - A non-status `/Status` while updating does not make the player
    NotAPlayer.
  - `Client::Timing` gets `upgradeCheckIntervalMs` (default one hour), so
    tests can shorten it.
- **Never sent:** `upgrade=this` or `all`. A test asserts that only
  `upgrade=check` reaches `/upgrade`.

## App

- **Notice:** one line on Home under the presets (canvas F1); a "Settings" pulley item on Home opens
  `qml/pages/PlayerSettingsPage.qml` (F2), which pushes `PlayerWebPage.qml` (F3).
- **`qml/pages/PlayerWebPage.qml`:** a Silica page with a `WebView` from
  `Sailfish.WebView 1.0` on `http://<host>/upgrade`, and a PageHeader with
  the player's name.
  - It is generic for one player's own pages, so a later decision about the
    settings tree can reuse it without a second web view.
- **Packaging:** `Permissions=Internet;Audio;WebView`, and
  `Requires: sailfish-components-webview-qt5`. Before building, check
  upstream practice for the Sailfish WebView in a Harbour app:
  - the `WebView` permission's exact name and prompt text in the target;
  - whether the requirement is `sailfish-components-webview-qt5` or
    `qml(Sailfish.WebView)`, as the target's packages provide;
  - the initialisation the WebView documentation asks for, if any.

## Tests

- **`FakePlayer`:**
  - `/upgrade?upgrade=check` answers set by the test;
  - a `/SyncStatus` with an `UpgradeStatusStage*` root;
  - a silent period.
- **Client tests:** for acceptance criteria 1 and 2.
- **Runtime QML test:** the client's upgrade properties reach JavaScript.
- **Device check:** acceptance criterion 5.

## Risks

- Firmware may change the undocumented `upgrade=check` answer. Swell then
  shows no notice and nothing breaks.
- The WebView adds a permission prompt and a dependency.
- A real update can only be checked when the NAD is offered one.

## Revision 2026-11-25: starting the update (draft)

- **Library (`Client`):**
  - `Q_INVOKABLE void startUpgrade(const QString &version)`, a
    `StartState { None, Checking, Sent }` member (the record of spec
    revision item 2), and a signal `upgradeStartFailed(QString reason)`.
  - `startUpgrade` returns at once unless the conditions of revision item 1
    hold and the state is None. It sets Checking and reads a check of its
    own through `fetch()` (not `checkUpgrade()`, which can skip when a check
    is in flight). On the answer it re-tests every condition; only then it
    sets Sent and sends `this` with the 60 s deadline. The answer to `this`
    is not used.
  - The one place that builds `/upgrade` queries takes the action
    (`check` or `this`) from the code, never from QML.
  - `enterUpdating()` gets the origin (start, `inProgress`, stage root), and
    a single-shot timer for the five-minute bound, started only for a start.
    Evidence of updating stops it. `leaveUpdating` clears Sent only after
    evidence (stage root seen or another firmware version).
  - `Client::Timing` gets `upgradeStartBoundMs`, so tests can shorten it.
- **App:**
  - `qml/pages/UpgradeDialog.qml`: a Silica `Dialog` (qmldir of the 5.1.0.11
    target, `Dialog 1.0 Dialog.qml`, `DialogHeader 1.0`) with the version and
    the warnings; `onAccepted` calls `startUpgrade(version)`.
  - `PlayerSettingsPage.qml` and the Home notice push the dialog instead of
    `PlayerWebPage.qml`; texts for starting, updating and "not started".
  - The client's start state reaches QML as a property with NOTIFY.
  - If the maintainer chooses to remove the WebView: `PlayerWebPage.qml`,
    `upgradePage()`, `Permissions=…;WebView`, the three WebView requirements,
    `PKGCONFIG += qt5embedwidget` and its BuildRequires go; issue #32 is
    closed as no longer applicable.
- **Tests:** `tests/firmware`: the cases of acceptance criterion 7 with
  `FakePlayer` counting `upgrade=this`; the existing "only check is sent"
  test becomes "only check, and this only through a start".
  `tests/qmlruntime`: criterion 8.
- **Alternatives considered:**
  - Open the page without `noheader`: its button cannot work (no jQuery).
  - Send `upgrade=old` as that button would on port 80: seen only in a
    button that cannot run; meaning unknown. Rejected for `this`, which the
    official app uses (azzurro).
  - RemorsePopup instead of a Dialog: a countdown that runs out starts an
    irreversible update when the user looks away. Rejected.
  - No bound after a start: a player that ignored `this` would show
    "Updating" until the app restarts.
