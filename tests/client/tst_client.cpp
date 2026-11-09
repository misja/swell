// SPDX-License-Identifier: MIT
// BluOS::Client against the in-process FakePlayer: long polling, the
// one-second rule, commands, /SyncStatus refresh, availability and artwork.
#include <QtTest>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "client.h"
#include "fakeplayer.h"

using namespace BluOS;

namespace {

QByteArray status(const QByteArray &etag, const QByteArray &title, const QByteArray &syncStat = "s1",
                  const QByteArray &state = "play")
{
    return "<status etag=\"" + etag + "\"><state>" + state + "</state><title1>" + title
            + "</title1><title2>Nobody</title2><title3>Invented</title3>"
              "<image>/Artwork?service=LocalMusic&amp;album=Invented</image>"
              "<secs>10</secs><totlen>200</totlen><syncStat>" + syncStat + "</syncStat></status>";
}

QByteArray syncStatus(const QByteArray &name, const QByteArray &syncStat = "s1", int volume = 30,
                      const QByteArray &inner = QByteArray())
{
    return "<SyncStatus etag=\"" + syncStat + "\" syncStat=\"" + syncStat + "\" name=\"" + name
            + "\" modelName=\"Test Model\" volume=\"" + QByteArray::number(volume)
            + "\" mute=\"0\" id=\"127.0.0.1:11000\">" + inner + "</SyncStatus>";
}

Client::Timing fast()
{
    Client::Timing t;
    t.pollTimeoutSecs = 1;
    t.deadlineGraceMs = 2000;
    t.commandTimeoutMs = 3000;
    t.minIntervalMs = 300;
    t.backoffInitialMs = 200;
    t.backoffMaxMs = 400;
    return t;
}

} // namespace

class TstClient : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void firstContactThenLongPoll();
    void longPollReturnsOnChange();
    void unchangedTimeoutKeepsState();
    void oneSecondRule();
    void commands();
    void volumeBurstCollapses();
    void fixedVolumeIgnoresSetVolume();
    void syncStatusRefetchedOnSyncStatChange();
    void secondaryUsesItsOwnSyncVolume();
    void offlineThenRecovered();
    void notAPlayer();
    void artworkUrlAndRedirect();
    void presetsFetchedAndRefetchedOnPrid();
    void playPreset();
    void playInputIsSentByteForByte_data();
    void playInputIsSentByteForByte();
    void playInputRejectsUnsafeUrl();
    void errorBodyIsACommandFailure();
    void errorBodyWithoutMessage();
    void aStoppedClientAnswersLater();
    void handlersGoWithTheirContext();
    void inputsFetched();
    void skipFollowsTheSource();
    void muteIsNotLostInAVolumeBurst();
    void presetsRefetchIsBounded();
    void serverErrorIsOfflineNot404();
    void statusPollingOffStopsStatusReads();

private:
    void startClient(const Client::Timing &timing = fast());

    QNetworkAccessManager *m_nam = nullptr;
    FakePlayer *m_player = nullptr;
    Client *m_client = nullptr;
};

void TstClient::init()
{
    m_nam = new QNetworkAccessManager(this);
    m_player = new FakePlayer(this);
    QVERIFY(m_player->listen());
    m_player->rememberPort();
    m_player->setStatus(status("e1", "Track A"));
    m_player->serve(QStringLiteral("/SyncStatus"), syncStatus("Living Room"));
    m_player->serve(QStringLiteral("/Volume"), "<volume db=\"-30\" mute=\"0\" etag=\"v1\">30</volume>");
    for (const char *path : { "/Play", "/Pause", "/Skip", "/Back" })
        m_player->serve(QString::fromLatin1(path), "<state>play</state>");
}

void TstClient::cleanup()
{
    delete m_client;
    m_client = nullptr;
    delete m_player;
    m_player = nullptr;
    delete m_nam;
    m_nam = nullptr;
}

void TstClient::startClient(const Client::Timing &timing)
{
    m_client = new Client(m_nam, QStringLiteral("127.0.0.1"), m_player->port());
    m_client->setTiming(timing);
    m_client->start();
}

void TstClient::firstContactThenLongPoll()
{
    startClient();
    QTRY_VERIFY(m_client->online());
    QTRY_COMPARE(m_client->name(), QStringLiteral("Living Room"));
    QCOMPARE(m_client->title1(), QStringLiteral("Track A"));
    QCOMPARE(m_client->volume(), 30);
    QVERIFY(m_client->playing());
    QCOMPARE(m_client->duration(), 200);

    QTRY_VERIFY(m_player->requests(QStringLiteral("/Status")).size() >= 2);
    const auto polls = m_player->requests(QStringLiteral("/Status"));
    QVERIFY(!polls.at(0).query.hasQueryItem(QStringLiteral("etag")));
    QCOMPARE(polls.at(1).query.queryItemValue(QStringLiteral("etag")), QStringLiteral("e1"));
    QCOMPARE(polls.at(1).query.queryItemValue(QStringLiteral("timeout")), QStringLiteral("1"));
}

void TstClient::longPollReturnsOnChange()
{
    Client::Timing t = fast();
    t.pollTimeoutSecs = 10;
    startClient(t);
    QTRY_VERIFY(m_player->requests(QStringLiteral("/Status")).size() >= 2);  // now held
    QSignalSpy changed(m_client, &Client::statusChanged);
    QElapsedTimer elapsed;
    elapsed.start();
    m_player->setStatus(status("e2", "Track B"));
    QTRY_COMPARE(m_client->title1(), QStringLiteral("Track B"));
    QVERIFY2(elapsed.elapsed() < 5000, "a held poll must answer on change, not at its timeout");
    QCOMPARE(changed.size(), 1);
}

void TstClient::unchangedTimeoutKeepsState()
{
    startClient();
    QTRY_VERIFY(m_player->requests(QStringLiteral("/Status")).size() >= 2);
    QSignalSpy changed(m_client, &Client::statusChanged);
    QSignalSpy availability(m_client, &Client::availabilityChanged);
    // Two 1 s timeouts with the same etag.
    QTRY_VERIFY_WITH_TIMEOUT(m_player->requests(QStringLiteral("/Status")).size() >= 4, 10000);
    QCOMPARE(changed.size(), 0);
    QCOMPARE(availability.size(), 0);
    QVERIFY(m_client->online());
    QCOMPARE(m_client->title1(), QStringLiteral("Track A"));
}

void TstClient::oneSecondRule()
{
    // A server that changes on every poll must still not be asked more often
    // than once per interval; this uses the real 1000 ms interval.
    Client::Timing t = fast();
    t.minIntervalMs = 1000;
    startClient(t);
    QTimer churn;
    int n = 0;
    connect(&churn, &QTimer::timeout, this, [this, &n]() {
        m_player->setStatus(status("c" + QByteArray::number(++n), "Churn"));
    });
    churn.start(50);
    QTRY_VERIFY_WITH_TIMEOUT(m_player->requests(QStringLiteral("/Status")).size() >= 4, 10000);
    churn.stop();
    const auto polls = m_player->requests(QStringLiteral("/Status"));
    for (int i = 1; i < polls.size(); ++i) {
        const qint64 gap = polls.at(i).atMs - polls.at(i - 1).atMs;
        QVERIFY2(gap >= 990, qPrintable(QStringLiteral("gap %1 ms").arg(gap)));
    }
}

void TstClient::commands()
{
    startClient();
    QTRY_VERIFY(m_client->online());
    m_client->play();
    m_client->pause();
    m_client->skip();
    m_client->back();
    m_client->setMuted(true);
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Play")).size(), 1);
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Pause")).size(), 1);
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Skip")).size(), 1);
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Back")).size(), 1);
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Volume")).size(), 1);
    QCOMPARE(m_player->requests(QStringLiteral("/Volume")).first().query.queryItemValue(QStringLiteral("mute")),
             QStringLiteral("1"));
}

void TstClient::volumeBurstCollapses()
{
    startClient();
    QTRY_COMPARE(m_client->volume(), 30);
    for (int level : { 10, 20, 30, 40, 50 })
        m_client->setVolume(level);
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Volume")).size(), 2);
    QTest::qWait(fast().minIntervalMs * 2);
    const auto sent = m_player->requests(QStringLiteral("/Volume"));
    QCOMPARE(sent.size(), 2);
    QCOMPARE(sent.at(0).query.queryItemValue(QStringLiteral("level")), QStringLiteral("10"));
    QCOMPARE(sent.at(1).query.queryItemValue(QStringLiteral("level")), QStringLiteral("50"));
    QVERIFY(sent.at(1).atMs - sent.at(0).atMs >= fast().minIntervalMs - 10);
}

void TstClient::fixedVolumeIgnoresSetVolume()
{
    m_player->serve(QStringLiteral("/SyncStatus"), syncStatus("Fixed", "s1", -1));
    startClient();
    QTRY_VERIFY(m_client->fixedVolume());
    m_client->setVolume(50);
    QTest::qWait(500);
    QCOMPARE(m_player->requests(QStringLiteral("/Volume")).size(), 0);
}

void TstClient::syncStatusRefetchedOnSyncStatChange()
{
    startClient();
    QTRY_COMPARE(m_client->name(), QStringLiteral("Living Room"));
    const int before = m_player->requests(QStringLiteral("/SyncStatus")).size();
    m_player->serve(QStringLiteral("/SyncStatus"), syncStatus("Renamed Room", "s2", 44));
    m_player->setStatus(status("e2", "Track A", "s2"));
    QTRY_COMPARE(m_client->name(), QStringLiteral("Renamed Room"));
    QCOMPARE(m_client->volume(), 44);
    QVERIFY(m_player->requests(QStringLiteral("/SyncStatus")).size() > before);
}

void TstClient::secondaryUsesItsOwnSyncVolume()
{
    // A grouped secondary's /Status copies the primary's; volume comes from /SyncStatus.
    m_player->setStatus("<status etag=\"e1\"><state>play</state><volume>80</volume>"
                        "<syncStat>s1</syncStat></status>");
    m_player->serve(QStringLiteral("/SyncStatus"),
                    syncStatus("Kitchen", "s1", 12, "<master port=\"11000\">192.0.2.10</master>"));
    startClient();
    QTRY_VERIFY(m_client->syncStatus().namesPrimary());
    QCOMPARE(m_client->volume(), 12);
}

void TstClient::offlineThenRecovered()
{
    startClient();
    QTRY_VERIFY(m_client->online());
    const quint16 port = m_player->port();
    m_player->close();
    QTRY_COMPARE_WITH_TIMEOUT(m_client->availability(), Client::Offline, 8000);
    QVERIFY(m_player->listen(port));
    m_player->setStatus(status("e9", "Back Again"));
    QTRY_COMPARE_WITH_TIMEOUT(m_client->availability(), Client::Online, 8000);
    QTRY_COMPARE(m_client->title1(), QStringLiteral("Back Again"));
}

void TstClient::notAPlayer()
{
    m_player->serve(QStringLiteral("/Status"), "<html><body>A router</body></html>", "text/html");
    startClient();
    QTRY_COMPARE(m_client->availability(), Client::NotAPlayer);
    QVERIFY(!m_client->online());
}

void TstClient::artworkUrlAndRedirect()
{
    startClient();
    QTRY_VERIFY(m_client->online());
    const QUrl art = m_client->artworkUrl();
    QCOMPARE(art.host(), QStringLiteral("127.0.0.1"));
    QCOMPARE(art.port(), int(m_player->port()));
    QCOMPARE(art.path(), QStringLiteral("/Artwork"));
    QCOMPARE(QUrlQuery(art).queryItemValue(QStringLiteral("followRedirects")), QStringLiteral("1"));
    QCOMPARE(QUrlQuery(art).queryItemValue(QStringLiteral("album")), QStringLiteral("Invented"));
    QCOMPARE(m_client->resolveImage(QStringLiteral("https://cdn.example/a.jpg")),
             QUrl(QStringLiteral("https://cdn.example/a.jpg")));

    // A player that redirects anyway (to its library port) is followed.
    FakePlayer library;
    QVERIFY(library.listen());
    library.serve(QStringLiteral("/Artwork"), "PNGDATA", "image/png");
    QUrl to;
    to.setScheme(QStringLiteral("http"));
    to.setHost(QStringLiteral("127.0.0.1"));
    to.setPort(library.port());
    to.setPath(QStringLiteral("/Artwork"));
    m_player->redirect(QStringLiteral("/Artwork"), to);
    QNetworkRequest request(art);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QNetworkReply *reply = m_nam->get(request);
    QSignalSpy finished(reply, &QNetworkReply::finished);
    QVERIFY(finished.wait(5000));
    QCOMPARE(reply->error(), QNetworkReply::NoError);
    QCOMPARE(reply->readAll(), QByteArray("PNGDATA"));
    reply->deleteLater();
}

void TstClient::presetsFetchedAndRefetchedOnPrid()
{
    m_player->setStatus("<status etag=\"e1\"><state>stop</state><prid>1</prid></status>");
    m_player->serve(QStringLiteral("/Presets"),
                    "<presets prid=\"1\"><preset id=\"5\" name=\"North Sea FM\" image=\"/Artwork?id=5\"/></presets>");
    startClient();
    QTRY_COMPARE(m_client->presets().size(), 1);
    const Preset first = m_client->presets().first();
    QCOMPARE(first.id, 5);
    QCOMPARE(first.image, QStringLiteral("/Artwork?id=5"));
    QCOMPARE(QUrlQuery(m_client->resolveImage(first.image)).queryItemValue(QStringLiteral("followRedirects")),
             QStringLiteral("1"));
    const int before = m_player->requests(QStringLiteral("/Presets")).size();

    m_player->serve(QStringLiteral("/Presets"),
                    "<presets prid=\"2\"><preset id=\"5\" name=\"North Sea FM\"/><preset id=\"9\" name=\"Dinner\"/></presets>");
    m_player->setStatus("<status etag=\"e2\"><state>stop</state><prid>2</prid></status>");
    QTRY_COMPARE(m_client->presets().size(), 2);
    QCOMPARE(m_player->requests(QStringLiteral("/Presets")).size(), before + 1);
}

void TstClient::playPreset()
{
    startClient();
    QTRY_VERIFY(m_client->online());
    m_client->playPreset(9);
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Preset")).size(), 1);
    QCOMPARE(m_player->requests(QStringLiteral("/Preset")).first().target, QByteArray("/Preset?id=9"));
}

void TstClient::playInputIsSentByteForByte_data()
{
    QTest::addColumn<QString>("url");
    // The shape of the API 11.1 example, and one with an encoded percent sign.
    QTest::newRow("capture") << "Capture%3Ahw%3Aimxspdif%2C0%2F1%2F25%2F2%3Fid%3Dinput1";
    QTest::newRow("percent") << "Capture%3Atest%2F100%25%2Fx";
    QTest::newRow("bluetooth") << "Capture%3Abluez%3Abluetooth";
}

void TstClient::playInputIsSentByteForByte()
{
    QFETCH(QString, url);
    startClient();
    QTRY_VERIFY(m_client->online());
    QSignalSpy failed(m_client, &Client::commandFailed);
    m_client->playInput(url);
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Play")).size(), 1);
    QCOMPARE(m_player->requests(QStringLiteral("/Play")).first().target, "/Play?url=" + url.toLatin1());
    QCOMPARE(failed.size(), 0);
}

void TstClient::playInputRejectsUnsafeUrl()
{
    startClient();
    QTRY_VERIFY(m_client->online());
    QSignalSpy failed(m_client, &Client::commandFailed);
    m_client->playInput(QStringLiteral("Capture%3Ax#frag"));
    m_client->playInput(QStringLiteral("Capture%3Ax&level=100"));
    m_client->playInput(QString());
    QCOMPARE(failed.size(), 3);
    QTest::qWait(300);
    QCOMPARE(m_player->requests(QStringLiteral("/Play")).size(), 0);
}

void TstClient::errorBodyIsACommandFailure()
{
    m_player->serve(QStringLiteral("/Preset"),
                    "<error><message>No such preset</message><detail>id 99</detail></error>");
    startClient();
    QTRY_VERIFY(m_client->online());
    QSignalSpy failed(m_client, &Client::commandFailed);
    m_client->playPreset(99);
    QTRY_COMPARE(failed.size(), 1);
    QCOMPARE(failed.first().at(0).toString(), QStringLiteral("/Preset"));
    QCOMPARE(failed.first().at(1).toString(), QStringLiteral("No such preset"));
}

void TstClient::errorBodyWithoutMessage()
{
    m_player->serve(QStringLiteral("/Preset"), "<error/>");
    startClient();
    QTRY_VERIFY(m_client->online());
    QSignalSpy failed(m_client, &Client::commandFailed);
    m_client->playPreset(99);
    QTRY_COMPARE(failed.size(), 1);
    QCOMPARE(failed.first().at(1).toString(), QStringLiteral("error"));
}

void TstClient::aStoppedClientAnswersLater()
{
    // Not started: the handler gets the failure, but not inside the call.
    m_client = new Client(m_nam, QStringLiteral("127.0.0.1"), m_player->port());
    int called = 0;
    Browse answer;
    m_client->fetchBrowse(QString(), QString(), this, [&](const Browse &b) {
        ++called;
        answer = b;
    });
    QCOMPARE(called, 0);
    QTRY_COMPARE(called, 1);
    QVERIFY(!answer.ok);
    QVERIFY(m_player->requests(QStringLiteral("/Browse")).isEmpty());
}

void TstClient::handlersGoWithTheirContext()
{
    m_player->serve(QStringLiteral("/Browse"), "<browse type=\"menu\"/>");
    m_player->delay(QStringLiteral("/Browse"), 200);
    m_player->serve(QStringLiteral("/Playlist"), "<playlist name=\"q\" id=\"1\" length=\"0\"/>");
    m_player->delay(QStringLiteral("/Playlist"), 200);
    startClient();
    QTRY_VERIFY(m_client->online());
    int kept = 0;
    int dropped = 0;
    QObject stays;
    auto *goes = new QObject;
    m_client->fetchBrowse(QString(), QString(), &stays, [&](const Browse &) { ++kept; });
    m_client->fetchBrowse(QString(), QString(), goes, [&](const Browse &) { ++dropped; });
    m_client->fetchQueueWindow(0, 9, &stays, [&](const Playlist &) { ++kept; });
    m_client->fetchQueueWindow(0, 9, goes, [&](const Playlist &) { ++dropped; });
    delete goes;
    QTRY_COMPARE(kept, 2);
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Browse")).size(), 2);
    QTest::qWait(400);
    QCOMPARE(dropped, 0);

    // Also when a stopped client answers on the next turn.
    m_client->stop();
    goes = new QObject;
    m_client->fetchBrowse(QString(), QString(), goes, [&](const Browse &) { ++dropped; });
    delete goes;
    QTest::qWait(50);
    QCOMPARE(dropped, 0);
}

void TstClient::inputsFetched()
{
    m_player->serve(QStringLiteral("/RadioBrowse"),
                    "<radiotime service=\"Capture\">"
                    "<item text=\"Optical Input\" inputType=\"spdif\" id=\"input1\" URL=\"Capture%3Ahw%3A1\" image=\"/images/o.png\"/>"
                    "<item text=\"Cloud\" id=\"c\" URL=\"Cloud%3Aplay\"/></radiotime>");
    startClient();
    QTRY_COMPARE(m_client->inputs().size(), 1);
    const Input in = m_client->inputs().first();
    QCOMPARE(in.text, QStringLiteral("Optical Input"));
    QCOMPARE(in.url, QStringLiteral("Capture%3Ahw%3A1"));
    QCOMPARE(m_player->requests(QStringLiteral("/RadioBrowse")).first().target,
             QByteArray("/RadioBrowse?service=Capture"));
}

void TstClient::skipFollowsTheSource()
{
    // A plain radio stream: nothing to skip.
    m_player->setStatus("<status etag=\"r1\"><state>stream</state><streamUrl>Radio:x</streamUrl></status>");
    startClient();
    QTRY_VERIFY(m_client->online());
    QVERIFY(!m_client->canSkip());
    QVERIFY(!m_client->canBack());
    m_client->skip();
    m_client->back();

    // A stream that offers skip (API 4.8): its own action URL is called.
    m_player->serve(QStringLiteral("/Action"), "<skip/>");
    m_player->setStatus("<status etag=\"r2\"><state>stream</state><streamUrl>Svc:x</streamUrl>"
                        "<actions><action name=\"back\"/>"
                        "<action name=\"skip\" url=\"/Action?service=Svc&amp;skip=42\"/></actions></status>");
    QTRY_VERIFY(m_client->canSkip());
    QVERIFY(!m_client->canBack());
    m_client->skip();
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Action")).size(), 1);
    QCOMPARE(m_player->requests(QStringLiteral("/Action")).first().target,
             QByteArray("/Action?service=Svc&skip=42"));

    // The queue: /Skip and /Back.
    m_player->setStatus("<status etag=\"q1\"><state>play</state></status>");
    QTRY_VERIFY(m_client->canBack());
    m_client->skip();
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Skip")).size(), 1);
    QCOMPARE(m_player->requests(QStringLiteral("/Back")).size(), 0);
}

void TstClient::muteIsNotLostInAVolumeBurst()
{
    startClient();
    QTRY_COMPARE(m_client->volume(), 30);
    m_client->setVolume(10);
    m_client->setMuted(true);
    m_client->setVolume(20);
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Volume")).size(), 3);
    const auto sent = m_player->requests(QStringLiteral("/Volume"));
    QStringList queries;
    for (const auto &r : sent)
        queries << QString::fromLatin1(r.target.mid(r.target.indexOf('?') + 1));
    QCOMPARE(queries.first(), QStringLiteral("level=10"));
    QVERIFY(queries.contains(QStringLiteral("mute=1")));
    QVERIFY(queries.contains(QStringLiteral("level=20")));
    // Still one request per resource per interval.
    for (int i = 1; i < sent.size(); ++i)
        QVERIFY(sent.at(i).atMs - sent.at(i - 1).atMs >= fast().minIntervalMs - 10);
}

void TstClient::presetsRefetchIsBounded()
{
    // /Presets reports a prid that never matches the status: no endless refetching.
    m_player->setStatus("<status etag=\"p1\"><state>stop</state><prid>7</prid></status>");
    m_player->serve(QStringLiteral("/Presets"), "<presets prid=\"999\"><preset id=\"1\" name=\"A\"/></presets>");
    startClient();
    QTRY_COMPARE(m_client->presets().size(), 1);
    QTest::qWait(fast().minIntervalMs * 5);
    QCOMPARE(m_player->requests(QStringLiteral("/Presets")).size(), 1);
}

void TstClient::serverErrorIsOfflineNot404()
{
    // A 5xx (e.g. during a firmware upgrade) is transient: Offline.
    m_player->failWith(QStringLiteral("/Status"), 503);
    startClient();
    QTRY_COMPARE(m_client->availability(), Client::Offline);
    delete m_client;
    m_client = nullptr;

    // A 404 means there is no control API here: not a player.
    m_player->failWith(QStringLiteral("/Status"), 404);
    startClient();
    QTRY_COMPARE(m_client->availability(), Client::NotAPlayer);
}

void TstClient::statusPollingOffStopsStatusReads()
{
    // Found in the 006 result review: the aborted poll used to reschedule
    // itself, so /Status went on being read.
    startClient();
    m_client->setSyncWatch(true);
    QTRY_VERIFY(m_client->online());
    QTRY_VERIFY(m_player->requests(QStringLiteral("/Status")).size() >= 2);  // the long poll is held
    m_client->setStatusPolling(false);
    QTest::qWait(300);
    const int before = m_player->requests(QStringLiteral("/Status")).size();
    QTest::qWait(2000);
    QCOMPARE(m_player->requests(QStringLiteral("/Status")).size(), before);
    // The watch keeps the player online.
    QCOMPARE(m_client->availability(), Client::Online);

    // Switched on again, /Status is read again.
    m_client->setStatusPolling(true);
    QTRY_VERIFY(m_player->requests(QStringLiteral("/Status")).size() > before);
    QCOMPARE(m_client->availability(), Client::Online);
}

QTEST_GUILESS_MAIN(TstClient)
#include "tst_client.moc"
