// SPDX-License-Identifier: MIT
// Feature 006: the firmware check and a player that updates, against the
// in-process FakePlayer. The documents follow azzurro's observations and the
// answer of a real player (spec.md); none of this is in the API.
#include <QtTest>
#include <QNetworkAccessManager>

#include "client.h"
#include "fakeplayer.h"

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
    void upgradePage();
    void inProgressMeansUpdating();
    void anUpdateRunsThroughToTheNewVersion();
    void aStageRootIsNotNotAPlayer();
    void stopDuringACheck();
    void stillAPlayerWhileDownloading();
    void noLongPollOnTheWatchWhileUpdating();
    void checksCanBeSwitchedOff();

private:
    void startClient(const Client::Timing &timing = fast());

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

void TstFirmware::upgradePage()
{
    m_client = new Client(m_nam, QStringLiteral("192.0.2.7"), 11000);
    QCOMPARE(m_client->upgradePage(), QUrl(QStringLiteral("http://192.0.2.7/upgrade?noheader=1")));
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

QTEST_MAIN(TstFirmware)
#include "tst_firmware.moc"
