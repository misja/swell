// SPDX-License-Identifier: MIT
// Deleting a preset (spec 008) against a FakePlayer that answers
// /SetPreset?id=N as the NAD did in the capture of 2026-11-26
// (specs/008-preset-delete/evidence.md): 200 with the whole list without
// that preset, prid one higher in /Presets and /Status, the other ids kept.
#include <QtTest>
#include <QNetworkAccessManager>

#include "client.h"
#include "fakeplayer.h"
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
    void offlineSendsNothing();
    void twoDeletesAreTwoRequests();
    void aFailedDeleteIsReported_data();
    void aFailedDeleteIsReported();

private:
    void publish();
    void startClient();
    QList<int> ids() const;
    int deletes() const;

    QNetworkAccessManager *m_nam = nullptr;
    FakePlayer *m_player = nullptr;
    Client *m_client = nullptr;
    QMap<int, QByteArray> m_presets;   // id -> name
    int m_prid = 0;
};

void TstPresets::init()
{
    m_nam = new QNetworkAccessManager(this);
    m_player = new FakePlayer(this);
    QVERIFY(m_player->listen());
    m_presets.clear();
    for (int id : { 1, 2, 3, 7, 8 })
        m_presets.insert(id, "Station " + QByteArray::number(id));
    m_prid = 4;
    m_player->serve(QStringLiteral("/SyncStatus"),
                    "<SyncStatus etag=\"s\" syncStat=\"s\" name=\"Den\" volume=\"20\"/>");
    // As captured: the delete answers the whole list, prid one higher.
    m_player->handle(QStringLiteral("/SetPreset"), [this](const QUrlQuery &q) {
        const QStringList keys = { QStringLiteral("id") };
        QStringList given;
        for (const auto &item : q.queryItems())
            given << item.first;
        if (given == keys)
            m_presets.remove(q.queryItemValue(QStringLiteral("id")).toInt());
        ++m_prid;
        publish();
        QByteArray xml = "<presets prid=\"" + QByteArray::number(m_prid) + "\">";
        for (auto it = m_presets.cbegin(); it != m_presets.cend(); ++it)
            xml += "<preset id=\"" + QByteArray::number(it.key()) + "\" name=\"" + it.value()
                    + "\" url=\"Invented:" + QByteArray::number(it.key()) + "\"/>";
        return xml + "</presets>";
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

void TstPresets::publish()
{
    QByteArray xml = "<presets prid=\"" + QByteArray::number(m_prid) + "\">";
    for (auto it = m_presets.cbegin(); it != m_presets.cend(); ++it)
        xml += "<preset id=\"" + QByteArray::number(it.key()) + "\" name=\"" + it.value()
                + "\" url=\"Invented:" + QByteArray::number(it.key()) + "\"/>";
    m_player->serve(QStringLiteral("/Presets"), xml + "</presets>");
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
}

QList<int> TstPresets::ids() const
{
    QList<int> list;
    for (const Preset &p : m_client->presets())
        list << p.id;
    return list;
}

int TstPresets::deletes() const
{
    return m_player->requests(QStringLiteral("/SetPreset")).size();
}

void TstPresets::deleteSendsOneRequestAndTheListFollows()
{
    startClient();
    QSignalSpy failed(m_client, &Client::commandFailed);
    QSignalSpy refused(m_client, &Client::presetDeleteRefused);
    m_client->deletePreset(7, m_client->prid());
    QTRY_COMPARE(deletes(), 1);
    QCOMPARE(m_player->requests(QStringLiteral("/SetPreset")).first().target, QByteArray("/SetPreset?id=7"));
    // The list comes from the prid change; 8 keeps its id (a gap at 7).
    QTRY_COMPARE(ids(), QList<int>({ 1, 2, 3, 8 }));
    QCOMPARE(m_client->prid(), QStringLiteral("5"));
    QVERIFY(failed.isEmpty());
    QVERIFY(refused.isEmpty());
}

void TstPresets::aChangedListIsNotTouched()
{
    // Ids are reused: once another controller changed the list, id 7 may be
    // another preset. Nothing is sent.
    startClient();
    const QString seen = m_client->prid();
    m_presets.remove(7);
    m_presets.insert(7, "Someone else's");
    ++m_prid;
    publish();
    QTRY_COMPARE(m_client->prid(), QString::number(m_prid));
    QSignalSpy refused(m_client, &Client::presetDeleteRefused);
    m_client->deletePreset(7, seen);
    QCOMPARE(refused.size(), 1);
    QCOMPARE(refused.first().first().toBool(), true);
    QTest::qWait(300);   // nothing follows, which no signal shows
    QCOMPARE(deletes(), 0);
}

void TstPresets::offlineSendsNothing()
{
    startClient();
    const QString seen = m_client->prid();
    m_client->stop();
    m_player->failWith(QStringLiteral("/Status"), 500);
    m_player->failWith(QStringLiteral("/SyncStatus"), 500);
    m_client->start();
    QTRY_VERIFY(!m_client->online());
    QSignalSpy refused(m_client, &Client::presetDeleteRefused);
    m_client->deletePreset(7, seen);
    QCOMPARE(refused.size(), 1);
    QCOMPARE(refused.first().first().toBool(), false);
    QCOMPARE(deletes(), 0);
}

void TstPresets::twoDeletesAreTwoRequests()
{
    // Not merged like a volume burst; paced per path.
    startClient();
    const QString seen = m_client->prid();
    m_client->deletePreset(7, seen);
    m_client->deletePreset(8, seen);
    QTRY_COMPARE(deletes(), 2);
    const auto sent = m_player->requests(QStringLiteral("/SetPreset"));
    QCOMPARE(sent.at(0).target, QByteArray("/SetPreset?id=7"));
    QCOMPARE(sent.at(1).target, QByteArray("/SetPreset?id=8"));
    QVERIFY(sent.at(1).atMs - sent.at(0).atMs >= fastTiming().minIntervalMs - 50);
    QTRY_COMPARE(ids(), QList<int>({ 1, 2, 3 }));
}

void TstPresets::aFailedDeleteIsReported_data()
{
    // Invented: none of these replies was measured (spec 008, "Not measured").
    QTest::addColumn<int>("http");
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<QString>("error");
    QTest::newRow("http error") << 500 << QByteArray() << QStringLiteral("no answer");
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
    if (http)
        m_player->failWith(QStringLiteral("/SetPreset"), http);
    else
        m_player->handle(QStringLiteral("/SetPreset"), [body](const QUrlQuery &) { return body; });
    QSignalSpy failed(m_client, &Client::commandFailed);
    m_client->deletePreset(7, m_client->prid());
    QTRY_COMPARE(failed.size(), 1);
    QCOMPARE(failed.first().at(0).toString(), QStringLiteral("/SetPreset"));
    QCOMPARE(failed.first().at(1).toString(), error);
    QCOMPARE(ids(), QList<int>({ 1, 2, 3, 7, 8 }));
}

QTEST_GUILESS_MAIN(TstPresets)
#include "tst_presets.moc"
