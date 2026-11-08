// SPDX-License-Identifier: MIT
// Grouping (spec 005) against FakeHousehold: roles from both sides, grouping
// and ungrouping, group volume, and keeping a secondary's volume current.
#include <QtTest>
#include <QNetworkAccessManager>

#include "client.h"
#include "fakehousehold.h"

using namespace BluOS;

class TstGroup : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void rolesFromBothSides();
    void aPrimaryNamingItselfIsStillPrimary();
    void addOne();
    void addSeveralOneAtATime();
    void aSilentlyFailedAddIsReported();
    void removeOne();
    void removeSeveral();
    void removingThePrimaryOfThree();
    void groupVolumeIsKeptApartFromOwnVolume();
    void aSecondarysVolumeNeedsTheSyncWatch();
    void theVolumeAnswerShowsAtOnce();
    void aLonePlayerHasNoSyncLongPoll();

private:
    Client *client(int i);

    QNetworkAccessManager *m_nam = nullptr;
    FakeHousehold *m_house = nullptr;
    QList<Client *> m_clients;
};

static Client::Timing fast()
{
    Client::Timing t;
    t.pollTimeoutSecs = 1;
    t.syncPollTimeoutSecs = 2;
    t.deadlineGraceMs = 2000;
    t.commandTimeoutMs = 3000;
    t.minIntervalMs = 200;
    t.backoffInitialMs = 200;
    t.backoffMaxMs = 400;
    return t;
}

void TstGroup::init()
{
    m_nam = new QNetworkAccessManager(this);
    m_house = new FakeHousehold(3, this);
    m_house->setName(0, QStringLiteral("Living Room"));
    m_house->setName(1, QStringLiteral("Kitchen"));
    m_house->setName(2, QStringLiteral("Studio"));
}

void TstGroup::cleanup()
{
    qDeleteAll(m_clients);
    m_clients.clear();
    delete m_house;
    m_house = nullptr;
    delete m_nam;
    m_nam = nullptr;
}

Client *TstGroup::client(int i)
{
    auto *c = new Client(m_nam, QStringLiteral("127.0.0.1"), m_house->port(i));
    c->setTiming(fast());
    c->start();
    m_clients.append(c);
    return c;
}

void TstGroup::rolesFromBothSides()
{
    m_house->group(0, { 1, 2 });
    Client *lead = client(0);
    Client *member = client(1);
    QTRY_COMPARE(lead->groupRole(), Client::Primary);
    QCOMPARE(lead->secondaryIds(), QStringList({ m_house->id(1), m_house->id(2) }));
    QCOMPARE(lead->groupName(), QStringLiteral("Living Room + 2"));
    QTRY_COMPARE(member->groupRole(), Client::Secondary);
    QCOMPARE(member->primaryId(), m_house->id(0));
    QVERIFY(member->secondaryIds().isEmpty());
}

void TstGroup::aPrimaryNamingItselfIsStillPrimary()
{
    // The section 2.2 example: <master> on the primary, naming itself.
    m_house->setPrimaryNamesItself(true);
    m_house->group(0, { 1 });
    Client *lead = client(0);
    QTRY_COMPARE(lead->groupRole(), Client::Primary);
    QVERIFY(lead->syncStatus().isSecondary());   // the parser alone would say so
    QVERIFY(lead->primaryId().isEmpty());
}

void TstGroup::addOne()
{
    Client *lead = client(0);
    QTRY_VERIFY(lead->online());
    QSignalSpy failed(lead, &Client::commandFailed);
    lead->addSecondaries({ m_house->id(1) });
    QVERIFY(lead->groupBusy());
    QTRY_COMPARE(m_house->leaderOf(1), 0);
    QTRY_COMPARE(lead->groupRole(), Client::Primary);
    QTRY_VERIFY(!lead->groupBusy());
    QCOMPARE(failed.size(), 0);
    const auto requests = m_house->player(0)->requests(QStringLiteral("/AddSlave"));
    QCOMPARE(requests.size(), 1);
    QCOMPARE(requests.first().target,
             QByteArray("/AddSlave?slave=127.0.0.1&port=") + QByteArray::number(m_house->port(1)));
}

void TstGroup::addSeveralOneAtATime()
{
    Client *lead = client(0);
    QTRY_VERIFY(lead->online());
    m_house->player(0)->delay(QStringLiteral("/AddSlave"), 300);
    lead->addSecondaries({ m_house->id(1), m_house->id(2) });
    QTRY_COMPARE(m_house->player(0)->requests(QStringLiteral("/AddSlave")).size(), 1);
    QTest::qWait(150);   // the first is not answered yet: no second request
    QCOMPARE(m_house->player(0)->requests(QStringLiteral("/AddSlave")).size(), 1);
    QTRY_COMPARE(m_house->player(0)->requests(QStringLiteral("/AddSlave")).size(), 2);
    QTRY_COMPARE(m_house->leaderOf(2), 0);
    QCOMPARE(m_house->leaderOf(1), 0);
    QTRY_VERIFY(!lead->groupBusy());
}

void TstGroup::aSilentlyFailedAddIsReported()
{
    m_house->refuseToJoin(2);
    Client *lead = client(0);
    QTRY_VERIFY(lead->online());
    QSignalSpy failed(lead, &Client::commandFailed);
    lead->addSecondaries({ m_house->id(2) });
    QTRY_COMPARE(failed.size(), 1);
    QCOMPARE(failed.first().at(0).toString(), QStringLiteral("/AddSlave"));
    QCOMPARE(m_house->leaderOf(2), -1);
}

void TstGroup::removeOne()
{
    m_house->group(0, { 1 });
    Client *lead = client(0);
    QTRY_COMPARE(lead->groupRole(), Client::Primary);
    lead->removeSecondaries({ m_house->id(1) });
    QTRY_COMPARE(m_house->leaderOf(1), -1);
    // The answer is the primary's /SyncStatus, taken over at once.
    QTRY_COMPARE(lead->groupRole(), Client::Alone);
    QCOMPARE(m_house->player(0)->requests(QStringLiteral("/RemoveSlave")).first().target,
             QByteArray("/RemoveSlave?slave=127.0.0.1&port=") + QByteArray::number(m_house->port(1)));
}

void TstGroup::removeSeveral()
{
    m_house->group(0, { 1, 2 });
    Client *lead = client(0);
    QTRY_COMPARE(lead->groupRole(), Client::Primary);
    lead->removeSecondaries({ m_house->id(1), m_house->id(2) });
    QTRY_COMPARE(lead->groupRole(), Client::Alone);
    QCOMPARE(m_house->player(0)->requests(QStringLiteral("/RemoveSlave")).first().target,
             QByteArray("/RemoveSlave?slaves=127.0.0.1,127.0.0.1&ports=")
             + QByteArray::number(m_house->port(1)) + "," + QByteArray::number(m_house->port(2)));
}

void TstGroup::removingThePrimaryOfThree()
{
    // Inferred, not documented: the primary removes itself and the other two
    // stay together (spec 005). Checked against the household's model only.
    m_house->group(0, { 1, 2 });
    Client *lead = client(0);
    Client *next = client(1);
    QTRY_COMPARE(lead->groupRole(), Client::Primary);
    lead->removeSecondaries({ m_house->id(0) });
    QTRY_COMPARE(lead->groupRole(), Client::Alone);
    QTRY_COMPARE(next->groupRole(), Client::Primary);
    QCOMPARE(next->secondaryIds(), QStringList({ m_house->id(2) }));
}

void TstGroup::groupVolumeIsKeptApartFromOwnVolume()
{
    m_house->group(0, { 1, 2 });
    Client *lead = client(0);
    QTRY_COMPARE(lead->groupRole(), Client::Primary);
    lead->setGroupVolume(40);
    lead->setVolume(12);   // same path, other key: neither replaces the other
    QTRY_COMPARE(m_house->player(0)->requests(QStringLiteral("/Volume")).size(), 2);
    const auto requests = m_house->player(0)->requests(QStringLiteral("/Volume"));
    QCOMPARE(requests.at(0).target, QByteArray("/Volume?tell_slaves=1&level=40"));
    QCOMPARE(requests.at(1).target, QByteArray("/Volume?level=12"));
    QCOMPARE(m_house->volume(1), 40);
    QCOMPARE(m_house->volume(2), 40);
    QCOMPARE(m_house->volume(0), 12);
}

void TstGroup::aSecondarysVolumeNeedsTheSyncWatch()
{
    // A secondary's /Status (and its syncStat) is its primary's copy, so a
    // change to its own volume elsewhere only shows in its own /SyncStatus.
    m_house->group(0, { 1, 2 });
    Client *watched = client(1);
    Client *unwatched = client(2);
    QTRY_COMPARE(watched->groupRole(), Client::Secondary);
    QTRY_COMPARE(unwatched->groupRole(), Client::Secondary);
    watched->setSyncWatch(true);
    QTest::qWait(300);
    m_house->setVolume(1, 77);
    m_house->setVolume(2, 66);
    QTRY_COMPARE(watched->volume(), 77);
    QTest::qWait(1500);
    QCOMPARE(unwatched->volume(), 30);
}

void TstGroup::theVolumeAnswerShowsAtOnce()
{
    Client *lone = client(0);
    QTRY_VERIFY(lone->syncStatus().ok);
    // /SyncStatus now answers slowly, so the new level can only come from the
    // /Volume answer within the time allowed.
    m_house->player(0)->delay(QStringLiteral("/SyncStatus"), 2000);
    lone->setVolume(55);
    QTRY_COMPARE_WITH_TIMEOUT(lone->volume(), 55, 1000);
}

void TstGroup::aLonePlayerHasNoSyncLongPoll()
{
    Client *lone = client(0);
    QTRY_VERIFY(lone->online());
    QTest::qWait(1500);
    for (const auto &r : m_house->player(0)->requests(QStringLiteral("/SyncStatus")))
        QVERIFY2(!r.query.hasQueryItem(QStringLiteral("etag")), r.target.constData());
}

QTEST_GUILESS_MAIN(TstGroup)
#include "tst_group.moc"
