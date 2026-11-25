// SPDX-License-Identifier: MIT
// Feature 006: the firmware check, starting an update, and a player that
// updates, against the in-process FakePlayer. The documents follow azzurro's
// observations and the answer of a real player (spec.md); none of this is in
// the API.
#include <QtTest>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "client.h"
#include "fakeplayer.h"
#include "playermodel.h"

using namespace BluOS;

namespace {

QByteArray status(const QByteArray &etag)
{
    return "<status etag=\"" + etag + "\"><state>stop</state><title1>Track</title1>"
           "<syncStat>s1</syncStat></status>";
}

QByteArray syncStatus(const QByteArray &version, const QByteArray &extra = QByteArray())
{
    return "<SyncStatus etag=\"" + version + "\" syncStat=\"s1\" name=\"Living Room\" "
           "modelName=\"Test Model\" volume=\"30\" version=\"" + version + "\" "
           "id=\"127.0.0.1:11000\"" + extra + "/>";
}

QByteArray check(bool available, bool inProgress = false, const QByteArray &version = QByteArray())
{
    QByteArray xml = "<upgrade inProgress=\"" + QByteArray(inProgress ? "true" : "false") + "\"";
    if (!version.isEmpty())
        xml += " version=\"" + version + "\"";
    return xml + " available=\"" + QByteArray(available ? "true" : "false") + "\">check</upgrade>";
}

bool isStart(const QUrlQuery &query)
{
    return query.queryItemValue(QStringLiteral("upgrade")) == QLatin1String("this");
}

// LSDP announcements, as in tests/model.
QByteArray lengthPrefixed(const QByteArray &bytes)
{
    return char(bytes.size()) + bytes;
}

QByteArray packet(const QByteArray &message)
{
    return QByteArray("\x06LSDP\x01", 6) + char(message.size() + 1) + message;
}

QByteArray announce(char node, char lastOctet, quint16 port)
{
    QByteArray m("A");
    m.append(lengthPrefixed(QByteArray("\x02\x00\x00\x00\x00", 5) + node));
    m.append(lengthPrefixed(QByteArray("\x7F\x00\x00", 3) + lastOctet));  // 127.0.0.x
    m.append(char(1));
    m.append(char(0)).append(char(1)).append(char(2));
    m.append(lengthPrefixed("port")).append(lengthPrefixed(QByteArray::number(port)));
    m.append(lengthPrefixed("name")).append(lengthPrefixed("Den"));
    return packet(m);
}

QByteArray deletion(char node)
{
    QByteArray m("D");
    m.append(lengthPrefixed(QByteArray("\x02\x00\x00\x00\x00", 5) + node));
    m.append(char(1)).append('\x00').append('\x01');
    return packet(m);
}

Client::Timing fast()
{
    Client::Timing t;
    t.pollTimeoutSecs = 1;
    t.deadlineGraceMs = 2000;
    t.commandTimeoutMs = 2000;
    t.minIntervalMs = 300;
    t.backoffInitialMs = 200;
    t.backoffMaxMs = 400;
    t.upgradeTimeoutMs = 2000;
    return t;
}

} // namespace

class TstFirmware : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void availableWithVersion();
    void notAvailable();
    void onlyEverAsks();
    void checkedHourly();
    void noAnswerKeepsWhatWasKnown();
    void notCheckedWithoutStatusPolling();
    void fixedSecondaryShowsNoUpdate();
    void inProgressMeansUpdating();
    void anUpdateRunsThroughToTheNewVersion();
    void aStageRootIsNotNotAPlayer();
    void stopDuringACheck();
    void stillAPlayerWhileDownloading();
    void noLongPollOnTheWatchWhileUpdating();
    void checksCanBeSwitchedOff();

    // Starting an update (revision 2026-11-25, AC7).
    void whyTheStartHasItsOwnSocket();
    void startSendsOnceAfterAFreshCheck();
    void startWithoutAVersion();
    void refusedByTheFreshCheck_data();
    void refusedByTheFreshCheck();
    void refusedWhenNotReady();
    void cancelledWhilePending_data();
    void cancelledWhilePending();
    void onlyOneStart();
    void noRetry_data();
    void noRetry();
    void recordSurvivesANewClient_data();
    void recordSurvivesANewClient();
    void updatingAfterAStart();
    void boundEndsOnlyOnAnAnswer();
    void unfinishedKeepsTheRecord();

private:
    void startClient(const Client::Timing &timing = fast());
    void startWithOffer();
    // How many starts reached the player; -1000 once anything else but a
    // check or a start reached /upgrade.
    int starts() const;

    QNetworkAccessManager *m_nam = nullptr;
    FakePlayer *m_player = nullptr;
    Client *m_client = nullptr;
};

void TstFirmware::init()
{
    m_nam = new QNetworkAccessManager(this);
    m_player = new FakePlayer(this);
    QVERIFY(m_player->listen());
    m_player->rememberPort();
    m_player->setStatus(status("e1"));
    m_player->serve(QStringLiteral("/SyncStatus"), syncStatus("4.18.18"));
    m_player->serve(QStringLiteral("/upgrade"), check(false));
}

void TstFirmware::cleanup()
{
    delete m_client;
    m_client = nullptr;
    delete m_player;
    m_player = nullptr;
    delete m_nam;
    m_nam = nullptr;
}

void TstFirmware::startClient(const Client::Timing &timing)
{
    m_client = new Client(m_nam, QStringLiteral("127.0.0.1"), m_player->port());
    m_client->setTiming(timing);
    m_client->start();
}

void TstFirmware::availableWithVersion()
{
    m_player->serve(QStringLiteral("/upgrade"), check(true, false, "4.18.20"));
    startClient();
    QSignalSpy changed(m_client, &Client::upgradeChanged);
    QTRY_VERIFY(m_client->upgradeAvailable());
    QCOMPARE(m_client->upgradeVersion(), QStringLiteral("4.18.20"));
    QCOMPARE(m_client->firmwareVersion(), QStringLiteral("4.18.18"));
    QCOMPARE(changed.count(), 1);
}

void TstFirmware::notAvailable()
{
    startClient();
    QTRY_VERIFY(m_client->online());
    QTRY_VERIFY(m_client->upgradeChecked());
    QCOMPARE(m_player->requests(QStringLiteral("/upgrade")).size(), 1);
    QVERIFY(!m_client->upgradeAvailable());
    QVERIFY(m_client->upgradeVersion().isEmpty());
}

void TstFirmware::onlyEverAsks()
{
    Client::Timing t = fast();
    t.upgradeCheckIntervalMs = 400;
    startClient(t);
    QTRY_VERIFY(m_client->online());
    m_client->checkUpgrade();
    QTRY_VERIFY(m_player->requests(QStringLiteral("/upgrade")).size() >= 3);
    for (const FakePlayer::Request &r : m_player->requests(QStringLiteral("/upgrade")))
        QCOMPARE(r.target, QByteArray("/upgrade?upgrade=check"));
}

void TstFirmware::checkedHourly()
{
    // The interval is the policy; tests shorten it.
    Client::Timing t = fast();
    t.upgradeCheckIntervalMs = 500;
    startClient(t);
    QTRY_VERIFY(m_client->online());
    QTRY_COMPARE(m_player->requests(QStringLiteral("/upgrade")).size(), 1);
    QVERIFY(!m_client->upgradeAvailable());
    m_player->serve(QStringLiteral("/upgrade"), check(true, false, "4.18.20"));
    QTRY_VERIFY(m_client->upgradeAvailable());

    // With the default interval, nothing more is asked for a while.
    delete m_client;
    m_client = nullptr;
    const int before = m_player->requests(QStringLiteral("/upgrade")).size();
    startClient();
    QTRY_VERIFY(m_client->online());
    QTRY_COMPARE(m_player->requests(QStringLiteral("/upgrade")).size(), before + 1);
    QTest::qWait(1500);
    QCOMPARE(m_player->requests(QStringLiteral("/upgrade")).size(), before + 1);
}

void TstFirmware::noAnswerKeepsWhatWasKnown()
{
    m_player->serve(QStringLiteral("/upgrade"), check(true, false, "4.18.20"));
    startClient();
    QTRY_VERIFY(m_client->upgradeAvailable());

    // A 404, a document that is not an answer, and no answer at all.
    m_player->failWith(QStringLiteral("/upgrade"), 404);
    m_client->checkUpgrade();
    QTRY_COMPARE(m_player->requests(QStringLiteral("/upgrade")).size(), 2);
    m_player->stopFailing(QStringLiteral("/upgrade"));
    m_player->serve(QStringLiteral("/upgrade"), "<html><body>Upgrade</body></html>", "text/html");
    QTest::qWait(400);
    m_client->checkUpgrade();
    QTRY_COMPARE(m_player->requests(QStringLiteral("/upgrade")).size(), 3);
    m_player->delay(QStringLiteral("/upgrade"), 5000);
    QTest::qWait(400);
    m_client->checkUpgrade();
    QTest::qWait(2500);   // past upgradeTimeoutMs
    QVERIFY(m_client->upgradeAvailable());
    QCOMPARE(m_client->upgradeVersion(), QStringLiteral("4.18.20"));
    QCOMPARE(m_client->availability(), Client::Online);
}

void TstFirmware::notCheckedWithoutStatusPolling()
{
    // A player shown beside the current one.
    m_client = new Client(m_nam, QStringLiteral("127.0.0.1"), m_player->port());
    m_client->setTiming(fast());
    m_client->setStatusPolling(false);
    m_client->setSyncWatch(true);
    m_client->start();
    QTRY_VERIFY(m_client->online());
    m_client->checkUpgrade();
    QTest::qWait(800);
    QVERIFY(m_player->requests(QStringLiteral("/upgrade")).isEmpty());

    // Once it becomes the current player, it is asked.
    m_client->setStatusPolling(true);
    QTRY_COMPARE(m_player->requests(QStringLiteral("/upgrade")).size(), 1);
}

void TstFirmware::fixedSecondaryShowsNoUpdate()
{
    m_player->serve(QStringLiteral("/SyncStatus"),
                    syncStatus("4.18.18", " zone=\"Lounge\" zoneSlave=\"true\""));
    m_player->serve(QStringLiteral("/upgrade"), check(true, false, "4.18.20"));
    startClient();
    QTRY_VERIFY(m_client->online());
    QTRY_VERIFY(m_client->syncStatus().fixedSecondary);
    QTest::qWait(500);
    QVERIFY(!m_client->upgradeAvailable());
    QVERIFY(m_client->upgradeVersion().isEmpty());
}

void TstFirmware::inProgressMeansUpdating()
{
    startClient();
    QTRY_VERIFY(m_client->online());
    // The user started the update on the player's page; then the page closes.
    m_player->serve(QStringLiteral("/upgrade"), check(false, true));
    m_player->serve(QStringLiteral("/SyncStatus"), "<UpgradeStatusStage1 name=\"Living Room\"/>");
    m_client->checkUpgrade();
    QTRY_COMPARE(m_client->availability(), Client::Updating);
    QVERIFY(!m_client->online());
}

void TstFirmware::anUpdateRunsThroughToTheNewVersion()
{
    m_player->serve(QStringLiteral("/upgrade"), check(true, false, "4.18.20"));
    startClient();
    QTRY_VERIFY(m_client->upgradeAvailable());
    m_player->serve(QStringLiteral("/upgrade"), check(false, true));
    m_client->checkUpgrade();
    QTRY_COMPARE(m_client->availability(), Client::Updating);

    // Silent while it restarts: still updating, not offline.
    QSignalSpy availability(m_client, &Client::availabilityChanged);
    m_player->close();
    QTest::qWait(1500);
    QCOMPARE(m_client->availability(), Client::Updating);
    QVERIFY(m_player->listen(m_player->port()));

    // Answering again, but as an updating player: /Status is not asked.
    m_player->serve(QStringLiteral("/SyncStatus"), "<UpgradeStatusStage2 name=\"Living Room\"/>");
    const int statusReads = m_player->requests(QStringLiteral("/Status")).size();
    QTest::qWait(1500);
    QCOMPARE(m_client->availability(), Client::Updating);
    QCOMPARE(m_player->requests(QStringLiteral("/Status")).size(), statusReads);
    QVERIFY(!m_player->requests(QStringLiteral("/SyncStatus")).isEmpty());
    for (const FakePlayer::Request &r : m_player->requests(QStringLiteral("/SyncStatus")))
        QVERIFY2(!r.query.hasQueryItem(QStringLiteral("timeout")), r.target.constData());
    QCOMPARE(availability.count(), 0);

    // Back with the new firmware: online, and the notice clears.
    m_player->serve(QStringLiteral("/SyncStatus"), syncStatus("4.18.20"));
    m_player->serve(QStringLiteral("/upgrade"), check(false));
    QTRY_COMPARE_WITH_TIMEOUT(m_client->availability(), Client::Online, 5000);
    QCOMPARE(m_client->firmwareVersion(), QStringLiteral("4.18.20"));
    QTRY_VERIFY(!m_client->upgradeAvailable());
}

void TstFirmware::aStageRootIsNotNotAPlayer()
{
    startClient();
    QTRY_VERIFY(m_client->online());
    // /Status during an update is not observed; whatever it answers, the
    // /SyncStatus read decides.
    m_player->serve(QStringLiteral("/SyncStatus"), "<UpgradeStatusStage1 name=\"Living Room\"/>");
    m_player->setStatus("<html><body>Upgrading</body></html>");
    QTRY_COMPARE(m_client->availability(), Client::Updating);

    // Without a stage root, the same /Status is not a player.
    delete m_client;
    m_client = nullptr;
    m_player->serve(QStringLiteral("/SyncStatus"), "<html/>");
    startClient();
    QTRY_COMPARE(m_client->availability(), Client::NotAPlayer);
}

void TstFirmware::stopDuringACheck()
{
    m_player->delay(QStringLiteral("/upgrade"), 500);
    m_player->serve(QStringLiteral("/upgrade"), check(true, true, "4.18.20"));
    startClient();
    QTRY_COMPARE(m_player->requests(QStringLiteral("/upgrade")).size(), 1);
    m_client->stop();
    QTest::qWait(1000);
    QVERIFY(!m_client->upgradeAvailable());
    QCOMPARE(m_client->availability(), Client::Online);
}

void TstFirmware::stillAPlayerWhileDownloading()
{
    // While the update downloads, /SyncStatus still answers as a player, but
    // the check says it is in progress: the player stays Updating, and the
    // check is asked at most once per probe, on the backoff.
    startClient();
    QTRY_VERIFY(m_client->online());
    m_player->serve(QStringLiteral("/upgrade"), check(false, true));
    m_client->checkUpgrade();
    QTRY_COMPARE(m_client->availability(), Client::Updating);
    QSignalSpy availability(m_client, &Client::availabilityChanged);
    const int checks = m_player->requests(QStringLiteral("/upgrade")).size();
    QTest::qWait(3000);
    QCOMPARE(m_client->availability(), Client::Updating);
    QCOMPARE(availability.count(), 0);
    // Backoff of at most 400 ms, plus the paced reads: well under one per 200 ms.
    QVERIFY(m_player->requests(QStringLiteral("/upgrade")).size() - checks <= 15);

    // Done: the check no longer says so, and the player is back.
    m_player->serve(QStringLiteral("/upgrade"), check(false));
    QTRY_COMPARE_WITH_TIMEOUT(m_client->availability(), Client::Online, 5000);
}

void TstFirmware::noLongPollOnTheWatchWhileUpdating()
{
    Client::Timing t = fast();
    t.syncPollTimeoutSecs = 1;
    startClient(t);
    m_client->setSyncWatch(true);
    QTRY_VERIFY(m_client->online());
    QTRY_VERIFY(!m_player->requests(QStringLiteral("/SyncStatus")).isEmpty());
    m_player->serve(QStringLiteral("/upgrade"), check(false, true));
    m_player->serve(QStringLiteral("/SyncStatus"), "<UpgradeStatusStage1 name=\"Living Room\"/>");
    m_client->checkUpgrade();
    QTRY_COMPARE(m_client->availability(), Client::Updating);
    const int before = m_player->requests(QStringLiteral("/SyncStatus")).size();
    QTest::qWait(2500);
    const QList<FakePlayer::Request> reads = m_player->requests(QStringLiteral("/SyncStatus"));
    QVERIFY(reads.size() > before);
    for (int i = before; i < reads.size(); ++i)
        QVERIFY2(!reads.at(i).query.hasQueryItem(QStringLiteral("timeout")), reads.at(i).target.constData());
}

void TstFirmware::checksCanBeSwitchedOff()
{
    m_client = new Client(m_nam, QStringLiteral("127.0.0.1"), m_player->port());
    m_client->setTiming(fast());
    m_client->setUpgradeChecks(false);
    m_client->start();
    QTRY_VERIFY(m_client->online());
    m_client->checkUpgrade();
    QTest::qWait(800);
    QVERIFY(m_player->requests(QStringLiteral("/upgrade")).isEmpty());
    QVERIFY(!m_client->upgradeChecked());
    m_client->setUpgradeChecks(true);
    QTRY_VERIFY(m_client->upgradeChecked());
    QCOMPARE(m_player->requests(QStringLiteral("/upgrade")).size(), 1);
}

int TstFirmware::starts() const
{
    int n = 0;
    for (const FakePlayer::Request &r : m_player->requests(QStringLiteral("/upgrade"))) {
        if (r.target == QByteArray("/upgrade?upgrade=this"))
            ++n;
        else if (r.target != QByteArray("/upgrade?upgrade=check"))
            return -1000;
    }
    return n;
}

void TstFirmware::startWithOffer()
{
    m_player->serve(QStringLiteral("/upgrade"), check(true, false, "4.18.20"));
    startClient();
    QTRY_VERIFY(m_client->online());
    QTRY_VERIFY(m_client->upgradeAvailable());
}

void TstFirmware::whyTheStartHasItsOwnSocket()
{
    // Qt 5.6's QNetworkAccessManager sends a request again when the server
    // reads it and closes without a byte (spec 006, Facts, Qt). If this ever
    // fails, the reason for OneShotGet is gone.
    m_player->silenceIf(QStringLiteral("/upgrade"), isStart, FakePlayer::Drop);
    QNetworkReply *reply = m_nam->get(QNetworkRequest(QUrl(
            QStringLiteral("http://127.0.0.1:%1/upgrade?upgrade=this").arg(m_player->port()))));
    QTRY_VERIFY(reply->isFinished());
    reply->deleteLater();
    QVERIFY2(starts() > 1, qPrintable(QString::number(starts())));
}

void TstFirmware::startSendsOnceAfterAFreshCheck()
{
    startWithOffer();
    const int before = m_player->requests(QStringLiteral("/upgrade")).size();
    QSignalSpy refused(m_client, &Client::upgradeStartRefused);
    m_client->startUpgrade(QStringLiteral("4.18.20"));
    QCOMPARE(m_client->upgradePhase(), Client::UpgradeChecking);
    QTRY_COMPARE(starts(), 1);
    // A check after the call, then the start.
    const QList<FakePlayer::Request> reads = m_player->requests(QStringLiteral("/upgrade"));
    QCOMPARE(reads.size(), before + 2);
    QCOMPARE(reads.at(before).target, QByteArray("/upgrade?upgrade=check"));
    QCOMPARE(reads.at(before + 1).target, QByteArray("/upgrade?upgrade=this"));
    QVERIFY(m_client->upgradeStarted());
    QCOMPARE(m_client->upgradePhase(), Client::UpgradeStarting);
    QCOMPARE(m_client->availability(), Client::Updating);
    QCOMPARE(m_client->upgradeStartFirmware(), QStringLiteral("4.18.18"));
    QTest::qWait(1500);
    QCOMPARE(starts(), 1);
    QCOMPARE(refused.count(), 0);
}

void TstFirmware::startWithoutAVersion()
{
    // Equality only: no version on the offer and none confirmed.
    m_player->serve(QStringLiteral("/upgrade"), check(true));
    startClient();
    QTRY_VERIFY(m_client->upgradeAvailable());
    m_client->startUpgrade(QString());
    QTRY_COMPARE(starts(), 1);
}

void TstFirmware::refusedByTheFreshCheck_data()
{
    QTest::addColumn<QString>("answer");
    QTest::newRow("not available") << "not available";
    QTest::newRow("in progress") << "in progress";
    QTest::newRow("another version") << "another version";
    QTest::newRow("404") << "404";
    QTest::newRow("malformed") << "malformed";
    QTest::newRow("no answer") << "no answer";
}

void TstFirmware::refusedByTheFreshCheck()
{
    QFETCH(QString, answer);
    startWithOffer();
    if (answer == QLatin1String("not available"))
        m_player->serve(QStringLiteral("/upgrade"), check(false));
    else if (answer == QLatin1String("in progress"))
        m_player->serve(QStringLiteral("/upgrade"), check(true, true, "4.18.20"));
    else if (answer == QLatin1String("another version"))
        m_player->serve(QStringLiteral("/upgrade"), check(true, false, "4.18.21"));
    else if (answer == QLatin1String("404"))
        m_player->failWith(QStringLiteral("/upgrade"), 404);
    else if (answer == QLatin1String("malformed"))
        m_player->serve(QStringLiteral("/upgrade"), "<html><body>Upgrade</body></html>", "text/html");
    else
        m_player->silenceIf(QStringLiteral("/upgrade"), [](const QUrlQuery &) { return true; },
                            FakePlayer::Hold);
    QSignalSpy refused(m_client, &Client::upgradeStartRefused);
    m_client->startUpgrade(QStringLiteral("4.18.20"));
    QTRY_COMPARE_WITH_TIMEOUT(refused.count(), 1, 4000);
    QCOMPARE(m_client->upgradePhase(), Client::NoUpgrade);
    QVERIFY(!m_client->upgradeStarted());
    QTest::qWait(500);
    QCOMPARE(starts(), 0);
}

void TstFirmware::refusedWhenNotReady()
{
    // Not the current player (checks off), not running, a fixed-group
    // secondary, offline, updating: refused at once, nothing sent.
    startWithOffer();
    QSignalSpy refused(m_client, &Client::upgradeStartRefused);
    m_client->setUpgradeChecks(false);
    m_client->startUpgrade(QStringLiteral("4.18.20"));
    QCOMPARE(refused.count(), 1);
    m_client->setUpgradeChecks(true);
    m_client->stop();
    m_client->startUpgrade(QStringLiteral("4.18.20"));
    QCOMPARE(refused.count(), 2);
    m_client->start();
    QTRY_VERIFY(m_client->online());

    m_player->serve(QStringLiteral("/upgrade"), check(true, true, "4.18.20"));
    m_client->checkUpgrade();
    QTRY_COMPARE(m_client->availability(), Client::Updating);
    m_client->startUpgrade(QStringLiteral("4.18.20"));
    QCOMPARE(refused.count(), 3);

    delete m_client;
    m_client = nullptr;
    m_player->serve(QStringLiteral("/upgrade"), check(true, false, "4.18.20"));
    m_player->serve(QStringLiteral("/SyncStatus"),
                    syncStatus("4.18.18", " zone=\"Lounge\" zoneSlave=\"true\""));
    startClient();
    QTRY_VERIFY(m_client->syncStatus().fixedSecondary);
    QSignalSpy refusedSecondary(m_client, &Client::upgradeStartRefused);
    m_client->startUpgrade(QStringLiteral("4.18.20"));
    QCOMPARE(refusedSecondary.count(), 1);

    delete m_client;
    m_client = nullptr;
    m_player->serve(QStringLiteral("/SyncStatus"), syncStatus("4.18.18"));
    startWithOffer();
    m_player->close();
    QTRY_COMPARE(m_client->availability(), Client::Offline);
    QSignalSpy refusedOffline(m_client, &Client::upgradeStartRefused);
    m_client->startUpgrade(QStringLiteral("4.18.20"));
    QCOMPARE(refusedOffline.count(), 1);
    QCOMPARE(starts(), 0);
}

void TstFirmware::cancelledWhilePending_data()
{
    QTest::addColumn<QString>("how");
    QTest::newRow("stop") << "stop";
    QTest::newRow("checks off") << "checks off";
    QTest::newRow("status polling off") << "status polling off";
    QTest::newRow("stop and start again") << "stop and start again";
}

void TstFirmware::cancelledWhilePending()
{
    QFETCH(QString, how);
    startWithOffer();
    m_player->delay(QStringLiteral("/upgrade"), 800);
    QSignalSpy refused(m_client, &Client::upgradeStartRefused);
    m_client->startUpgrade(QStringLiteral("4.18.20"));
    QCOMPARE(m_client->upgradePhase(), Client::UpgradeChecking);
    if (how == QLatin1String("stop")) {
        m_client->stop();
    } else if (how == QLatin1String("checks off")) {
        m_client->setUpgradeChecks(false);   // another player became current
    } else if (how == QLatin1String("status polling off")) {
        m_client->setStatusPolling(false);
    } else {
        m_client->stop();
        m_client->start();
    }
    QCOMPARE(m_client->upgradePhase(), Client::NoUpgrade);
    QCOMPARE(refused.count(), 1);
    QTest::qWait(2000);
    QCOMPARE(starts(), 0);
    QVERIFY(!m_client->upgradeStarted());
}

void TstFirmware::onlyOneStart()
{
    startWithOffer();
    QSignalSpy refused(m_client, &Client::upgradeStartRefused);
    m_client->startUpgrade(QStringLiteral("4.18.20"));
    m_client->startUpgrade(QStringLiteral("4.18.20"));   // while pending
    QCOMPARE(refused.count(), 1);
    QTRY_COMPARE(starts(), 1);
    m_client->startUpgrade(QStringLiteral("4.18.20"));   // after it was sent
    QCOMPARE(refused.count(), 2);
    QTest::qWait(1500);
    QCOMPARE(starts(), 1);
}

void TstFirmware::noRetry_data()
{
    QTest::addColumn<QString>("how");
    QTest::newRow("closed without a byte") << "drop";
    QTest::newRow("no answer before the deadline") << "hold";
    QTest::newRow("an error document") << "error";
}

void TstFirmware::noRetry()
{
    QFETCH(QString, how);
    if (how == QLatin1String("drop")) {
        m_player->silenceIf(QStringLiteral("/upgrade"), isStart, FakePlayer::Drop);
    } else if (how == QLatin1String("hold")) {
        m_player->silenceIf(QStringLiteral("/upgrade"), isStart, FakePlayer::Hold);
    } else {
        m_player->handle(QStringLiteral("/upgrade"), [](const QUrlQuery &q) -> QByteArray {
            return isStart(q) ? QByteArray("<error><message>busy</message></error>")
                              : check(true, false, "4.18.20");
        });
    }
    startWithOffer();
    m_client->startUpgrade(QStringLiteral("4.18.20"));
    QTRY_COMPARE(starts(), 1);
    // Past twice the deadline (upgradeTimeoutMs, 2 s).
    QTest::qWait(5000);
    QCOMPARE(starts(), 1);
    QVERIFY(m_client->upgradeStarted());
}

void TstFirmware::recordSurvivesANewClient_data()
{
    QTest::addColumn<QString>("how");
    QTest::newRow("address change") << "address change";
    QTest::newRow("lost and found") << "lost and found";
    QTest::newRow("removed and added") << "removed and added";
}

void TstFirmware::recordSurvivesANewClient()
{
    QFETCH(QString, how);
    m_player->serve(QStringLiteral("/upgrade"), check(true, false, "4.18.20"));
    PlayerModel model;
    model.setClientTiming(fast());
    const QString id = QStringLiteral("127.0.0.1:%1").arg(m_player->port());
    const QString node = QStringLiteral("02:00:00:00:00:0A");
    if (how == QLatin1String("removed and added")) {
        QVERIFY(model.addManual(id));
        model.setCurrentId(id);
    } else {
        model.restoreCurrent(id, node);
    }
    Client *first = model.current();
    QVERIFY(first);
    QTRY_VERIFY(first->online());
    QTRY_VERIFY(first->upgradeAvailable());
    first->startUpgrade(QStringLiteral("4.18.20"));
    QTRY_COMPARE(starts(), 1);

    if (how == QLatin1String("address change")) {
        model.discovery()->handleDatagram(announce('\x0A', 20, m_player->port()));
    } else if (how == QLatin1String("lost and found")) {
        QVERIFY(model.addManual(QStringLiteral("127.0.0.30:1")));
        model.setCurrentId(QStringLiteral("127.0.0.30:1"));
        model.discovery()->handleDatagram(deletion('\x0A'));
        QCOMPARE(model.indexOf(id), -1);
        model.discovery()->handleDatagram(announce('\x0A', 1, m_player->port()));
        model.setCurrentId(id);
    } else {
        model.removeManual(id);
        QCOMPARE(model.indexOf(id), -1);
        QVERIFY(model.addManual(id));
        model.setCurrentId(id);
    }
    Client *second = model.current();
    QVERIFY(second);
    QVERIFY(second != first);
    QVERIFY(second->upgradeStarted());
    QCOMPARE(second->upgradeStartFirmware(), QStringLiteral("4.18.18"));
    QSignalSpy refused(second, &Client::upgradeStartRefused);
    second->startUpgrade(QStringLiteral("4.18.20"));
    QCOMPARE(refused.count(), 1);
    QTest::qWait(1000);
    QCOMPARE(starts(), 1);
}

void TstFirmware::updatingAfterAStart()
{
    startWithOffer();
    m_client->startUpgrade(QStringLiteral("4.18.20"));
    QTRY_COMPARE(m_client->upgradePhase(), Client::UpgradeStarting);
    QCOMPARE(m_client->availability(), Client::Updating);
    const int syncReads = m_player->requests(QStringLiteral("/SyncStatus")).size();

    // The player still answers as before: no sign of an update yet, so
    // still Updating, read plainly.
    QSignalSpy availability(m_client, &Client::availabilityChanged);
    QTest::qWait(2000);
    QCOMPARE(m_client->availability(), Client::Updating);
    QCOMPARE(m_client->upgradePhase(), Client::UpgradeStarting);
    QCOMPARE(availability.count(), 0);
    const QList<FakePlayer::Request> reads = m_player->requests(QStringLiteral("/SyncStatus"));
    QVERIFY(reads.size() > syncReads + 1);
    for (int i = syncReads; i < reads.size(); ++i)
        QVERIFY2(!reads.at(i).query.hasQueryItem(QStringLiteral("timeout")), reads.at(i).target.constData());

    // Installing, then silent while it restarts.
    m_player->serve(QStringLiteral("/SyncStatus"), "<UpgradeStatusStage1 name=\"Living Room\"/>");
    QTRY_COMPARE(m_client->upgradePhase(), Client::UpgradeInstalling);
    m_player->close();
    QTRY_COMPARE(m_client->upgradePhase(), Client::UpgradeRestarting);
    QCOMPARE(m_client->availability(), Client::Updating);

    // Back on the new version: online, done, and the record is cleared.
    QVERIFY(m_player->listen(m_player->port()));
    m_player->serve(QStringLiteral("/SyncStatus"), syncStatus("4.18.20"));
    m_player->serve(QStringLiteral("/upgrade"), check(false));
    QTRY_COMPARE_WITH_TIMEOUT(m_client->availability(), Client::Online, 5000);
    QCOMPARE(m_client->upgradePhase(), Client::UpgradeDone);
    QVERIFY(!m_client->upgradeStarted());
    QCOMPARE(m_client->firmwareVersion(), QStringLiteral("4.18.20"));
    QTRY_VERIFY(!m_client->upgradeAvailable());
    QCOMPARE(starts(), 1);
}

void TstFirmware::boundEndsOnlyOnAnAnswer()
{
    Client::Timing t = fast();
    t.upgradeStartBoundMs = 1500;
    m_player->serve(QStringLiteral("/upgrade"), check(true, false, "4.18.20"));
    startClient(t);
    QTRY_VERIFY(m_client->upgradeAvailable());
    m_client->startUpgrade(QStringLiteral("4.18.20"));
    QTRY_COMPARE(m_client->upgradePhase(), Client::UpgradeStarting);

    // Silent past the bound: still Updating, nothing seen of an update.
    m_player->close();
    QTest::qWait(2500);
    QCOMPARE(m_client->availability(), Client::Updating);
    QCOMPARE(m_client->upgradePhase(), Client::UpgradeStarting);

    // An ordinary answer past the bound: the update did not start.
    QVERIFY(m_player->listen(m_player->port()));
    QTRY_COMPARE_WITH_TIMEOUT(m_client->availability(), Client::Online, 5000);
    QCOMPARE(m_client->upgradePhase(), Client::UpgradeNotStarted);
    QVERIFY(m_client->upgradeStarted());
    QSignalSpy refused(m_client, &Client::upgradeStartRefused);
    m_client->startUpgrade(QStringLiteral("4.18.20"));
    QCOMPARE(refused.count(), 1);
    QTest::qWait(1000);
    QCOMPARE(starts(), 1);
}

void TstFirmware::unfinishedKeepsTheRecord()
{
    startWithOffer();
    m_client->startUpgrade(QStringLiteral("4.18.20"));
    QTRY_COMPARE(m_client->upgradePhase(), Client::UpgradeStarting);
    m_player->serve(QStringLiteral("/upgrade"), check(true, true, "4.18.20"));
    QTRY_COMPARE(m_client->upgradePhase(), Client::UpgradeInstalling);
    // The check no longer says so, and the player is back on the same version.
    m_player->serve(QStringLiteral("/upgrade"), check(true, false, "4.18.20"));
    QTRY_COMPARE_WITH_TIMEOUT(m_client->availability(), Client::Online, 5000);
    QCOMPARE(m_client->upgradePhase(), Client::UpgradeUnfinished);
    QVERIFY(m_client->upgradeStarted());
    QCOMPARE(starts(), 1);
}

QTEST_MAIN(TstFirmware)
#include "tst_firmware.moc"
