// SPDX-License-Identifier: MIT
// Deleting, adding and moving presets (spec 008) against a FakePlayer that
// answers as the NAD did in the measurements of 2026-11-26 and 2026-11-27
// (specs/008-preset-delete/evidence.md): the whole list in the reply, prid
// one higher in /Presets and /Status, gaps kept.
#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>

#include "client.h"
#include "fakeplayer.h"
#include "sendlog.h"
#include "timing.h"

using namespace BluOS;

class TstPresets : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void deleteSendsOneRequestAndTheListFollows();
    void aChangedListIsNotTouched();
    void aListNotYetReadIsNotTouched();
    void offlineSendsNothing();
    void twoDeletesAreTwoRequests();
    void addTakesTheFirstFreeSlot();
    void aFailedDeleteIsReported_data();
    void aFailedDeleteIsReported();
    void anInputIsAdded();
    void anInputThatCannotBeSentIsNotAdded_data();
    void anInputThatCannotBeSentIsNotAdded();
    void aFailedInputAddIsReported_data();
    void aFailedInputAddIsReported();
    void anInputIsNotAddedToAListNotYetRead();
    void theCycleIsAsMeasured_data();
    void theCycleIsAsMeasured();
    void onlyPermutationsPass_data();
    void onlyPermutationsPass();
    void aMoveSendsOneRequestAndTheListFollows();
    void aChangedListIsRefusedByThePlayer();
    void aMoveIsRefusedBeforeSending_data();
    void aMoveIsRefusedBeforeSending();
    void oneMoveAtATime();
    void aFailedMoveIsReported_data();
    void aFailedMoveIsReported();

private:
    QByteArray list() const;
    void publish();
    void startClient();
    QList<int> ids() const;
    int setPresetRequests() const;

    SendLog *m_nam = nullptr;
    FakePlayer *m_player = nullptr;
    Client *m_client = nullptr;
    QMap<int, QByteArray> m_presets;   // id -> name
    int m_prid = 0;
};

void TstPresets::init()
{
    m_nam = new SendLog(this);
    m_player = new FakePlayer(this);
    QVERIFY(m_player->listen());
    m_presets.clear();
    for (int id : { 1, 2, 3, 7, 8 })
        m_presets.insert(id, "Station " + QByteArray::number(id));
    m_prid = 4;
    // The inputs as /RadioBrowse?service=Capture gives them (spec 008 facts);
    // the second with a URL that would not stay one parameter.
    m_player->serve(QStringLiteral("/RadioBrowse"),
                    "<radiotime service=\"Capture\">"
                    "<item text=\"Phono &amp; Co\" inputType=\"phono\" id=\"in5\""
                    " URL=\"Capture%3Ahw%3Asoundchassis%2C0%2F1%2F25%2F2%3Fid%3Dxdynamic-Source5\""
                    " image=\"/images/capture/ic_vinyl.png\"/>"
                    "<item text=\"Odd\" inputType=\"analog\" id=\"odd\" URL=\"Capture%3Ax&amp;level=100\"/>"
                    "</radiotime>");
    m_player->serve(QStringLiteral("/SyncStatus"),
                    "<SyncStatus etag=\"s\" syncStat=\"s\" name=\"Den\" volume=\"20\"/>");
    // As measured (evidence.md): an id-only request deletes (captured from
    // the official app); an add without an id takes the lowest free id (one
    // gap measured) and allows a station twice. Both answer the whole list
    // with prid one higher. In the capture /Status showed the new prid about
    // 40 ms after the reply; here it is published before. Anything else (an
    // add with an id, a full list) answers the list unchanged (invented).
    // As measured (evidence.md, 2026-11-27): /Presets/edit without ?prid is
    // 400, with an old prid 412; otherwise each move puts the preset of
    // `from` into `to`, a slot only vacated if no move fills it, so an
    // incomplete permutation deletes; 200 with the list, prid one higher.
    // The bodies of other errors are invented.
    m_player->handlePost(QStringLiteral("/Presets/edit"), [this](const QUrlQuery &q, const QByteArray &body) {
        if (!q.hasQueryItem(QStringLiteral("prid")))
            return qMakePair(400, QByteArray("no prid"));
        if (q.queryItemValue(QStringLiteral("prid")).toInt() != m_prid)
            return qMakePair(412, QByteArray());
        const QMap<int, QByteArray> before = m_presets;
        const QJsonArray ordering = QJsonDocument::fromJson(body).object().value(QStringLiteral("ordering")).toArray();
        for (const QJsonValue &m : ordering)
            m_presets.remove(m.toObject().value(QStringLiteral("from")).toInt());
        for (const QJsonValue &m : ordering)
            m_presets.insert(m.toObject().value(QStringLiteral("to")).toInt(),
                             before.value(m.toObject().value(QStringLiteral("from")).toInt()));
        ++m_prid;
        publish();
        return qMakePair(200, list());
    });
    m_player->handle(QStringLiteral("/SetPreset"), [this](const QUrlQuery &q) {
        if (q.queryItems().size() == 1 && q.hasQueryItem(QStringLiteral("id"))) {
            // An id that is not there: invented, not measured.
            m_presets.remove(q.queryItemValue(QStringLiteral("id")).toInt());
            ++m_prid;
            publish();
        } else if (!q.hasQueryItem(QStringLiteral("id")) && q.hasQueryItem(QStringLiteral("url"))) {
            int id = 1;
            while (m_presets.contains(id))
                ++id;
            m_presets.insert(id, q.queryItemValue(QStringLiteral("name"), QUrl::FullyDecoded).toUtf8());
            ++m_prid;
            publish();
        }
        return list();
    });
    publish();
}

void TstPresets::cleanup()
{
    delete m_client;
    m_client = nullptr;
    delete m_player;
    m_player = nullptr;
    delete m_nam;
    m_nam = nullptr;
}

QByteArray TstPresets::list() const
{
    QByteArray xml = "<presets prid=\"" + QByteArray::number(m_prid) + "\">";
    for (auto it = m_presets.cbegin(); it != m_presets.cend(); ++it)
        xml += "<preset id=\"" + QByteArray::number(it.key()) + "\" name=\"" + QString::fromUtf8(it.value()).toHtmlEscaped().toUtf8()
                + "\" url=\"Invented:" + QUrl::toPercentEncoding(QString::fromUtf8(it.value())) + "\"/>";
    return xml + "</presets>";
}

void TstPresets::publish()
{
    m_player->serve(QStringLiteral("/Presets"), list());
    m_player->setStatus("<status etag=\"p" + QByteArray::number(m_prid) + "\"><state>stop</state><prid>"
                        + QByteArray::number(m_prid) + "</prid><syncStat>s</syncStat></status>");
}

void TstPresets::startClient()
{
    m_client = new Client(m_nam, QStringLiteral("127.0.0.1"), m_player->port());
    m_client->setTiming(fastTiming());
    m_client->start();
    QTRY_VERIFY(m_client->online());
    QTRY_COMPARE(m_client->presets().size(), m_presets.size());
    QTRY_COMPARE(m_client->inputs().size(), 2);
}

QList<int> TstPresets::ids() const
{
    QList<int> list;
    for (const Preset &p : m_client->presets())
        list << p.id;
    return list;
}

int TstPresets::setPresetRequests() const
{
    return m_player->requests(QStringLiteral("/SetPreset")).size();
}

void TstPresets::deleteSendsOneRequestAndTheListFollows()
{
    startClient();
    QSignalSpy failed(m_client, &Client::commandFailed);
    QSignalSpy refused(m_client, &Client::presetDeleteRefused);
    m_client->deletePreset(7, m_client->presetsPrid());
    QTRY_COMPARE(setPresetRequests(), 1);
    QCOMPARE(m_player->requests(QStringLiteral("/SetPreset")).first().target, QByteArray("/SetPreset?id=7"));
    // The list comes from the prid change; 8 keeps its id (a gap at 7).
    QTRY_COMPARE(ids(), QList<int>({ 1, 2, 3, 8 }));
    QCOMPARE(m_client->presetsPrid(), QStringLiteral("5"));
    QVERIFY(failed.isEmpty());
    QVERIFY(refused.isEmpty());
}

void TstPresets::aChangedListIsNotTouched()
{
    // Ids are reused: once another controller changed the list, id 7 may be
    // another preset. Nothing is sent.
    startClient();
    const QString seen = m_client->presetsPrid();
    m_presets.remove(7);
    m_presets.insert(7, "Someone else's");
    ++m_prid;
    publish();
    // The list refreshes from the prid change.
    auto replaced = [this]() {
        for (const Preset &p : m_client->presets())
            if (p.id == 7 && p.name == QLatin1String("Someone else's"))
                return true;
        return false;
    };
    QTRY_VERIFY(replaced());
    QVERIFY(m_client->presetsCurrent());
    QSignalSpy refused(m_client, &Client::presetDeleteRefused);
    m_client->deletePreset(7, seen);
    QCOMPARE(refused.size(), 1);
    QCOMPARE(refused.first().first().toBool(), true);
    QTest::qWait(300);   // nothing follows, which no signal shows
    QCOMPARE(setPresetRequests(), 0);
}

void TstPresets::aListNotYetReadIsNotTouched()
{
    // /Status already reports a newer list that /Presets has not given yet:
    // the tiles show the old one, so nothing is sent.
    startClient();
    const QString seen = m_client->presetsPrid();
    m_player->failWith(QStringLiteral("/Presets"), 500);
    m_presets.insert(7, "Someone else's");
    ++m_prid;
    publish();
    QTRY_VERIFY(!m_client->presetsCurrent());
    QCOMPARE(m_client->presetsPrid(), seen);
    QSignalSpy refused(m_client, &Client::presetDeleteRefused);
    m_client->deletePreset(7, seen);
    QCOMPARE(refused.size(), 1);
    QCOMPARE(refused.first().first().toBool(), true);
    QCOMPARE(setPresetRequests(), 0);
}

void TstPresets::offlineSendsNothing()
{
    startClient();
    const QString seen = m_client->presetsPrid();
    m_client->stop();
    m_player->failWith(QStringLiteral("/Status"), 500);
    m_player->failWith(QStringLiteral("/SyncStatus"), 500);
    m_client->start();
    QTRY_VERIFY(!m_client->online());
    QSignalSpy refused(m_client, &Client::presetDeleteRefused);
    m_client->deletePreset(7, seen);
    QCOMPARE(refused.size(), 1);
    QCOMPARE(refused.first().first().toBool(), false);
    QCOMPARE(setPresetRequests(), 0);
}

void TstPresets::twoDeletesAreTwoRequests()
{
    // Not merged like a volume burst; paced per path.
    startClient();
    const QString seen = m_client->presetsPrid();
    m_client->deletePreset(7, seen);
    m_client->deletePreset(8, seen);
    QTRY_COMPARE(setPresetRequests(), 2);
    const auto sent = m_player->requests(QStringLiteral("/SetPreset"));
    QCOMPARE(sent.at(0).target, QByteArray("/SetPreset?id=7"));
    QCOMPARE(sent.at(1).target, QByteArray("/SetPreset?id=8"));
    const QList<qint64> times = m_nam->sentAt(QStringLiteral("/SetPreset"));
    QVERIFY(times.at(1) - times.at(0) >= fastTiming().minIntervalMs - 1);   // whole milliseconds
    QTRY_COMPARE(ids(), QList<int>({ 1, 2, 3 }));
}

void TstPresets::addTakesTheFirstFreeSlot()
{
    // Through runAction, with the player's own actionURL (spec 008).
    startClient();
    QSignalSpy done(m_client, &Client::actionDone);
    const QString add = QStringLiteral("/SetPreset?image=https%3A%2F%2Fimg.example%2Fx.jpg&name=RockIt%21"
                                       "&service=RadioParadise&url=RadioParadise%3A%2F2%3A20%2FRockIt%2521");
    m_client->runAction(add, QStringLiteral("preset"));
    QTRY_COMPARE(done.size(), 1);
    QCOMPARE(done.first().first().toString(), QStringLiteral("preset"));
    QCOMPARE(m_player->requests(QStringLiteral("/SetPreset")).size(), 1);
    QCOMPARE(QString::fromLatin1(m_player->requests(QStringLiteral("/SetPreset")).first().target), add);
    QTRY_COMPARE(ids(), QList<int>({ 1, 2, 3, 4, 7, 8 }));   // the gap at 4 filled
    m_client->runAction(add, QStringLiteral("preset"));        // the same station again
    QTRY_COMPARE(ids(), QList<int>({ 1, 2, 3, 4, 5, 7, 8 }));
    QCOMPARE(m_player->requests(QStringLiteral("/SetPreset")).size(), 2);
}

void TstPresets::aFailedDeleteIsReported_data()
{
    // Invented: none of these replies was measured (spec 008, "Not measured").
    QTest::addColumn<int>("http");   // -1: no answer within commandTimeoutMs
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<QString>("error");
    QTest::newRow("http error") << 500 << QByteArray() << QStringLiteral("no answer");
    QTest::newRow("timeout") << -1 << QByteArray() << QStringLiteral("no answer");
    QTest::newRow("error document") << 0 << QByteArray("<error><message>Nope</message></error>")
                                    << QStringLiteral("Nope");
    QTest::newRow("still listed") << 0
                                  << QByteArray("<presets prid=\"9\"><preset id=\"7\" name=\"x\" url=\"u\"/></presets>")
                                  << QStringLiteral("not deleted");
}

void TstPresets::aFailedDeleteIsReported()
{
    QFETCH(int, http);
    QFETCH(QByteArray, body);
    QFETCH(QString, error);
    startClient();
    if (http < 0)
        m_player->delay(QStringLiteral("/SetPreset"), fastTiming().commandTimeoutMs + 2000);
    else if (http)
        m_player->failWith(QStringLiteral("/SetPreset"), http);
    else
        m_player->handle(QStringLiteral("/SetPreset"), [body](const QUrlQuery &) { return body; });
    QSignalSpy failed(m_client, &Client::commandFailed);
    m_client->deletePreset(7, m_client->presetsPrid());
    QTRY_COMPARE_WITH_TIMEOUT(failed.size(), 1, fastTiming().commandTimeoutMs + 2000);
    QCOMPARE(failed.first().at(0).toString(), QStringLiteral("/SetPreset"));
    QCOMPARE(failed.first().at(1).toString(), error);
    QCOMPARE(ids(), QList<int>({ 1, 2, 3, 7, 8 }));
}

void TstPresets::anInputIsAdded()
{
    // The player offers no action for an input; Swell sends the parameters
    // /Services declares, as in the curl measurement (spec 008).
    startClient();
    QSignalSpy done(m_client, &Client::actionDone);
    QSignalSpy failed(m_client, &Client::commandFailed);
    m_client->addInputPreset(QStringLiteral("in5"));
    QTRY_COMPARE(done.size(), 1);
    QCOMPARE(done.first().first().toString(), QStringLiteral("preset"));
    QVERIFY(failed.isEmpty());
    const auto sent = m_player->requests(QStringLiteral("/SetPreset"));
    QCOMPARE(sent.size(), 1);
    // The URL byte for byte; the name and the player's relative image encoded.
    QCOMPARE(sent.first().target,
             QByteArray("/SetPreset?name=Phono%20%26%20Co"
                        "&url=Capture%3Ahw%3Asoundchassis%2C0%2F1%2F25%2F2%3Fid%3Dxdynamic-Source5"
                        "&image=%2Fimages%2Fcapture%2Fic_vinyl.png"));
    QTRY_COMPARE(ids(), QList<int>({ 1, 2, 3, 4, 7, 8 }));   // the first free slot
}

void TstPresets::anInputThatCannotBeSentIsNotAdded_data()
{
    QTest::addColumn<QString>("id");
    QTest::addColumn<QString>("error");
    QTest::newRow("unknown id") << QStringLiteral("gone") << QStringLiteral("unknown input");
    QTest::newRow("invalid URL") << QStringLiteral("odd") << QStringLiteral("invalid input URL");
}

void TstPresets::anInputThatCannotBeSentIsNotAdded()
{
    QFETCH(QString, id);
    QFETCH(QString, error);
    startClient();
    QSignalSpy failed(m_client, &Client::commandFailed);
    m_client->addInputPreset(id);
    QCOMPARE(failed.size(), 1);
    QCOMPARE(failed.first().at(0).toString(), QStringLiteral("/SetPreset"));
    QCOMPARE(failed.first().at(1).toString(), error);
    QTest::qWait(300);   // nothing follows, which no signal shows
    QCOMPARE(setPresetRequests(), 0);
}

void TstPresets::aFailedInputAddIsReported_data()
{
    // Invented: no failed add was measured (spec 008, "Not measured").
    QTest::addColumn<int>("http");   // -1: no answer within commandTimeoutMs
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<QString>("error");
    QTest::newRow("http error") << 500 << QByteArray() << QStringLiteral("no answer");
    QTest::newRow("timeout") << -1 << QByteArray() << QStringLiteral("no answer");
    QTest::newRow("error document") << 0 << QByteArray("<error><message>Nope</message></error>")
                                    << QStringLiteral("Nope");
    QTest::newRow("unchanged list") << 0 << QByteArray() << QStringLiteral("not added");
}

void TstPresets::aFailedInputAddIsReported()
{
    QFETCH(int, http);
    QFETCH(QByteArray, body);
    QFETCH(QString, error);
    startClient();
    if (http < 0) {
        m_player->delay(QStringLiteral("/SetPreset"), fastTiming().commandTimeoutMs + 2000);
    } else if (http) {
        m_player->failWith(QStringLiteral("/SetPreset"), http);
    } else {
        // An empty body here stands for the list as it was, prid unchanged.
        m_player->handle(QStringLiteral("/SetPreset"),
                         [this, body](const QUrlQuery &) { return body.isEmpty() ? list() : body; });
    }
    QSignalSpy done(m_client, &Client::actionDone);
    QSignalSpy failed(m_client, &Client::commandFailed);
    m_client->addInputPreset(QStringLiteral("in5"));
    QTRY_COMPARE_WITH_TIMEOUT(failed.size(), 1, fastTiming().commandTimeoutMs + 2000);
    QCOMPARE(failed.first().at(0).toString(), QStringLiteral("/SetPreset"));
    QCOMPARE(failed.first().at(1).toString(), error);
    QVERIFY(done.isEmpty());
    QCOMPARE(ids(), QList<int>({ 1, 2, 3, 7, 8 }));
}

void TstPresets::anInputIsNotAddedToAListNotYetRead()
{
    // /Status reports a newer list than /Presets gave: a reply's prid could
    // not tell an add from that change, so nothing is sent.
    startClient();
    m_player->failWith(QStringLiteral("/Presets"), 500);
    ++m_prid;
    publish();
    QTRY_VERIFY(!m_client->presetsCurrent());
    QSignalSpy failed(m_client, &Client::commandFailed);
    m_client->addInputPreset(QStringLiteral("in5"));
    QCOMPARE(failed.size(), 1);
    QCOMPARE(failed.first().at(1).toString(), QStringLiteral("presets not current"));
    QCOMPARE(setPresetRequests(), 0);
}

void TstPresets::theCycleIsAsMeasured_data()
{
    QTest::addColumn<QList<int>>("ids");
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("to");
    QTest::addColumn<QByteArray>("body");
    const QList<int> seven({ 1, 2, 3, 4, 5, 6, 7 });
    // The official app's bodies, byte for byte (evidence.md, 2026-11-27).
    QTest::newRow("7 to 2") << seven << 7 << 2
        << QByteArray("{\"ordering\":[{\"from\":7,\"to\":2},{\"from\":2,\"to\":3},{\"from\":3,\"to\":4},"
                      "{\"from\":4,\"to\":5},{\"from\":5,\"to\":6},{\"from\":6,\"to\":7}]}");
    QTest::newRow("2 to 6") << seven << 2 << 6
        << QByteArray("{\"ordering\":[{\"from\":3,\"to\":2},{\"from\":4,\"to\":3},{\"from\":5,\"to\":4},"
                      "{\"from\":6,\"to\":5},{\"from\":2,\"to\":6}]}");
    QTest::newRow("7 to 6") << seven << 7 << 6
        << QByteArray("{\"ordering\":[{\"from\":7,\"to\":6},{\"from\":6,\"to\":7}]}");
    QTest::newRow("6 to 7") << seven << 6 << 7
        << QByteArray("{\"ordering\":[{\"from\":7,\"to\":6},{\"from\":6,\"to\":7}]}");
    // Over the occupied slots: 9 empty, as measured with curl.
    QTest::newRow("across a gap") << QList<int>({ 1, 8, 10 }) << 10 << 8
        << QByteArray("{\"ordering\":[{\"from\":10,\"to\":8},{\"from\":8,\"to\":10}]}");
}

void TstPresets::theCycleIsAsMeasured()
{
    QFETCH(QList<int>, ids);
    QFETCH(int, from);
    QFETCH(int, to);
    QFETCH(QByteArray, body);
    m_presets.clear();
    for (int id : ids)
        m_presets.insert(id, "Station " + QByteArray::number(id));
    publish();
    startClient();
    QVERIFY(Client::isPermutation(Client::presetCycle(ids, from, to), ids));
    QSignalSpy failed(m_client, &Client::commandFailed);
    const QByteArray prid = m_client->presetsPrid().toUtf8();
    m_client->movePreset(from, to, m_client->presetsPrid());
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Presets/edit")).size(), 1);
    const FakePlayer::Request sent = m_player->requests(QStringLiteral("/Presets/edit")).first();
    QCOMPARE(sent.method, QByteArray("POST"));
    QCOMPARE(sent.contentType, QByteArray("application/json"));
    QCOMPARE(sent.target, "/Presets/edit?prid=" + prid);
    QCOMPARE(sent.body, body);
    QTRY_VERIFY(!m_client->presetMoving());
    QVERIFY(failed.isEmpty());
}

void TstPresets::onlyPermutationsPass_data()
{
    QTest::addColumn<QList<int>>("froms");
    QTest::addColumn<QList<int>>("tos");
    QTest::addColumn<bool>("ok");
    QTest::newRow("a swap") << QList<int>({ 7, 6 }) << QList<int>({ 6, 7 }) << true;
    // Measured: 9 to 8 alone deleted the preset in 8.
    QTest::newRow("one move") << QList<int>({ 7 }) << QList<int>({ 6 }) << false;
    QTest::newRow("a destination twice") << QList<int>({ 6, 7 }) << QList<int>({ 7, 7 }) << false;
    QTest::newRow("a source twice") << QList<int>({ 7, 7 }) << QList<int>({ 6, 7 }) << false;
    QTest::newRow("a source not there") << QList<int>({ 9, 6 }) << QList<int>({ 6, 9 }) << false;
    QTest::newRow("nothing") << QList<int>() << QList<int>() << false;
}

void TstPresets::onlyPermutationsPass()
{
    QFETCH(QList<int>, froms);
    QFETCH(QList<int>, tos);
    QFETCH(bool, ok);
    QList<Client::PresetMove> moves;
    for (int i = 0; i < froms.size(); ++i)
        moves.append({ froms.at(i), tos.at(i) });
    QCOMPARE(Client::isPermutation(moves, { 1, 2, 3, 6, 7 }), ok);
}

void TstPresets::aMoveSendsOneRequestAndTheListFollows()
{
    startClient();   // 1, 2, 3, 7, 8
    QSignalSpy failed(m_client, &Client::commandFailed);
    QSignalSpy refused(m_client, &Client::presetMoveRefused);
    QSignalSpy moving(m_client, &Client::presetMovingChanged);
    QSignalSpy version(m_client, &Client::presetsVersionChanged);
    m_client->movePreset(8, 2, m_client->presetsPrid());
    QVERIFY(m_client->presetMoving());
    QTRY_VERIFY(!m_client->presetMoving());
    // No second move on the old list: it is current again once read anew.
    QVERIFY(!version.isEmpty());
    QTRY_VERIFY(m_client->presetsCurrent());
    QCOMPARE(m_client->presetsPrid(), QString::number(m_prid));
    QCOMPARE(moving.size(), 2);
    QCOMPARE(m_player->requests(QStringLiteral("/Presets/edit")).size(), 1);
    // The list follows from the prid change; the ids stay, the names move.
    auto names = [this]() {
        QStringList list;
        for (const Preset &p : m_client->presets())
            list << QString::number(p.id) + QLatin1Char('=') + p.name.section(QLatin1Char(' '), 1);
        return list.join(QLatin1Char(' '));
    };
    QTRY_COMPARE(names(), QStringLiteral("1=1 2=8 3=2 7=3 8=7"));
    QVERIFY(failed.isEmpty());
    QVERIFY(refused.isEmpty());
}

void TstPresets::aChangedListIsRefusedByThePlayer()
{
    // Another controller changed the list after Swell read it: the player
    // answers 412 (measured) and nothing changes.
    startClient();
    m_player->handlePost(QStringLiteral("/Presets/edit"),
                         [](const QUrlQuery &, const QByteArray &) { return qMakePair(412, QByteArray()); });
    QSignalSpy failed(m_client, &Client::commandFailed);
    QSignalSpy refused(m_client, &Client::presetMoveRefused);
    m_client->movePreset(8, 2, m_client->presetsPrid());
    QTRY_COMPARE(refused.size(), 1);
    QCOMPARE(refused.first().first().toBool(), true);
    QVERIFY(failed.isEmpty());
    QVERIFY(!m_client->presetMoving());
    QCOMPARE(ids(), QList<int>({ 1, 2, 3, 7, 8 }));
}

void TstPresets::aMoveIsRefusedBeforeSending_data()
{
    QTest::addColumn<QString>("why");
    QTest::addColumn<int>("refusal");   // -1: none; 0: refused, offline; 1: refused, changed
    QTest::newRow("offline") << "offline" << 0;
    QTest::newRow("not current") << "not current" << 1;
    QTest::newRow("an old prid") << "old prid" << 1;
    QTest::newRow("an id not there") << "unknown id" << 1;
    QTest::newRow("the same place") << "same" << -1;
}

void TstPresets::aMoveIsRefusedBeforeSending()
{
    QFETCH(QString, why);
    QFETCH(int, refusal);
    startClient();
    QString prid = m_client->presetsPrid();
    int from = 8;
    int to = 2;
    if (why == QLatin1String("offline")) {
        m_client->stop();
        m_player->failWith(QStringLiteral("/Status"), 500);
        m_player->failWith(QStringLiteral("/SyncStatus"), 500);
        m_client->start();
        QTRY_VERIFY(!m_client->online());
    } else if (why == QLatin1String("not current")) {
        m_player->failWith(QStringLiteral("/Presets"), 500);
        ++m_prid;
        publish();
        QTRY_VERIFY(!m_client->presetsCurrent());
    } else if (why == QLatin1String("old prid")) {
        prid = QStringLiteral("3");
    } else if (why == QLatin1String("unknown id")) {
        from = 5;
    } else {
        to = from;
    }
    QSignalSpy refused(m_client, &Client::presetMoveRefused);
    QSignalSpy failed(m_client, &Client::commandFailed);
    m_client->movePreset(from, to, prid);
    if (refusal < 0) {
        QVERIFY(refused.isEmpty());
    } else {
        QCOMPARE(refused.size(), 1);
        QCOMPARE(refused.first().first().toBool(), refusal == 1);
    }
    QTest::qWait(300);   // nothing follows, which no signal shows
    QVERIFY(failed.isEmpty());
    QCOMPARE(m_player->requests(QStringLiteral("/Presets/edit")).size(), 0);
}

void TstPresets::oneMoveAtATime()
{
    startClient();
    const QString prid = m_client->presetsPrid();
    QSignalSpy refused(m_client, &Client::presetMoveRefused);
    m_client->movePreset(8, 2, prid);
    m_client->movePreset(7, 1, prid);
    QCOMPARE(refused.size(), 1);
    QCOMPARE(refused.first().first().toBool(), false);
    QTRY_VERIFY(!m_client->presetMoving());
    QTest::qWait(300);
    QCOMPARE(m_player->requests(QStringLiteral("/Presets/edit")).size(), 1);
}

void TstPresets::aFailedMoveIsReported_data()
{
    // Invented, except the 400 (measured without ?prid; forced here).
    QTest::addColumn<int>("http");   // -1: no answer within commandTimeoutMs; 0: the body with 200
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<QString>("error");
    QTest::newRow("400") << 400 << QByteArray("no prid") << QStringLiteral("HTTP 400");
    QTest::newRow("500") << 500 << QByteArray() << QStringLiteral("HTTP 500");
    QTest::newRow("timeout") << -1 << QByteArray() << QStringLiteral("no answer");
    QTest::newRow("error document") << 0 << QByteArray("<error><message>Nope</message></error>")
                                    << QStringLiteral("Nope");
    // A list that lost a preset, as an incomplete permutation does.
    QTest::newRow("a preset lost") << 0
        << QByteArray("<presets prid=\"5\"><preset id=\"1\" name=\"a\" url=\"Invented:Station%201\"/>"
                      "<preset id=\"2\" name=\"b\" url=\"Invented:Station%208\"/></presets>")
        << QStringLiteral("not moved");
}

void TstPresets::aFailedMoveIsReported()
{
    QFETCH(int, http);
    QFETCH(QByteArray, body);
    QFETCH(QString, error);
    startClient();
    if (http < 0) {
        m_player->delay(QStringLiteral("/Presets/edit"), fastTiming().commandTimeoutMs + 2000);
    } else {
        const int status = http ? http : 200;
        m_player->handlePost(QStringLiteral("/Presets/edit"), [status, body](const QUrlQuery &, const QByteArray &) {
            return qMakePair(status, body);
        });
    }
    QSignalSpy failed(m_client, &Client::commandFailed);
    QSignalSpy refused(m_client, &Client::presetMoveRefused);
    QSignalSpy moveFailed(m_client, &Client::presetMoveFailed);
    m_client->movePreset(8, 2, m_client->presetsPrid());
    QTRY_COMPARE_WITH_TIMEOUT(failed.size(), 1, fastTiming().commandTimeoutMs + 2000);
    QCOMPARE(moveFailed.size(), 1);
    QCOMPARE(failed.first().at(0).toString(), QStringLiteral("/Presets/edit"));
    QCOMPARE(failed.first().at(1).toString(), error);
    QVERIFY(refused.isEmpty());
    QVERIFY(!m_client->presetMoving());
}

QTEST_GUILESS_MAIN(TstPresets)
#include "tst_presets.moc"
