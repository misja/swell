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

## Revision 2026-11-25: starting the update

After the scope/design review (approve with changes, evidence.md).

- **PlayerModel:** a set of players with a start record (node id, else
  `host:port`), for the session. It hands the record to every Client it
  creates for that player (`createClient`), and receives it from the Client
  when set. Clients outside PlayerModel (tests) keep their own.
- **Client:**
  - `Q_INVOKABLE void startUpgrade(const QString &version)`;
    `upgradePhase` (enum: None, Checking, Starting, Installing, Restarting,
    NotAnswering, Updated, NotStarted) with NOTIFY; `upgradeStarted`
    (the record) with NOTIFY.
  - The start's check uses `fetch()` with a generation number; `stop()`,
    `setUpgradeChecks(false)`, `setStatusPolling(false)` bump it and reset
    Checking to None. On the answer, every condition is tested again; then
    the record is set and `this` is sent in the same turn.
  - **Sending `this`:** `bluos/oneshot.{h,cpp}`, a QTcpSocket that writes one
    `GET /upgrade?upgrade=this HTTP/1.1` with `Connection: close`, reads
    until close or the 60 s deadline, and never reconnects. It reports only
    whether an answer came. This is an own mechanism, needed because
    QNetworkAccessManager resends (spec, Facts); the PR names it.
    Alternative for the maintainer: QNetworkAccessManager and the risk
    recorded.
  - **Updating after a start:** `probeSyncStatus` gets one rule: with the
    record set and no evidence yet, an ordinary `SyncStatus` keeps Updating,
    except past the bound (`Timing::upgradeStartBoundMs`, five minutes),
    where it ends Updating with phase NotStarted. Evidence (stage root,
    `inProgress="true"`, another `firmwareVersion`) sets a flag; with it,
    today's rules apply. Another version clears the record.
  - Nothing else in `enterUpdating`/`leaveUpdating` changes.
- **App:**
  - `qml/pages/UpgradeDialog.qml`: `Dialog` with `DialogHeader`, a property
    `player` set at push, the version and the warnings; `canAccept` as in
    the spec; `onAccepted: player.startUpgrade(version)`.
  - `PlayerSettingsPage.qml` and the Home notice push the dialog with the
    current player; they show the phase and hide the button with a record
    or while Updating.
  - New strings in Dutch; existing ones reused.
- **WebView removal:** `qml/pages/PlayerWebPage.qml`, `Client::upgradePage()`,
  `tests/qmlruntime/Probe.qml` l. 63, `tst_firmware` `upgradePage`,
  `Permissions=…;WebView`, three `Requires`, `PKGCONFIG += qt5embedwidget`
  and its BuildRequires, the comments in `client.h`/`client.cpp` that say the
  client only asks, the strings, and household.py's update page; issue #32
  closed as no longer applicable.
- **Tests:**
  - `FakePlayer`: a mode that reads a request and closes without a byte.
  - `tests/firmware`: acceptance criterion 7; `onlyEverAsks` stays for a
    client that is not asked to start.
  - `tests/model`: the record across Client recreation.
  - `tests/qmlruntime`: criterion 8.
  - `tests/demo/household.py --firmware`: accept `this` once (then the
    existing update sequence), log a second as REFUSED; the port 80 page
    goes.
- **Alternatives considered:**
  - The page without `noheader`: its button cannot work (no jQuery).
  - `upgrade=old` on port 80, as that button would: seen only in a button
    that cannot run; meaning unknown.
  - RemorsePopup: a countdown that runs out starts an irreversible update
    when the user looks away.
  - No bound: a player that ignored `this` would show Updating until the
    app restarts.
  - Silence as evidence: one dropped read would end Updating mid-download.
