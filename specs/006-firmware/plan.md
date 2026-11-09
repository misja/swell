# Plan — 006 firmware updates

Status: revised 2026-11-09 after the scope/design review; the spec's scope
and findings H1, M1–M7, L1–L5 apply. Open: Misja's approvals listed in tasks.md.

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
