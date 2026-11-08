// SPDX-License-Identifier: MIT
// Grouping (spec 005) against FakeHousehold: roles from both sides, grouping
// and ungrouping, group volume, and keeping a secondary's volume current.
#include <QtTest>
#include <QNetworkAccessManager>

#include "client.h"
#include "playermodel.h"
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
    void theCurrentPlayerFollowsItsPrimary();
    void choosingAMemberChoosesItsPrimary();
    void watchingTheGroupKeepsMembersCurrent();
    void joinAndLeaveThroughTheModel();
    void aStaleGroupLoopDoesNotRecurse();
    void thePlayerListShowsWhoCanJoin();
    void aMembershipChangeElsewhereShows();
    void aLonePlayersRequestsAreUnchanged();
    void aReconnectingSecondary();
    void groupAndMemberMute();
    void pairHalvesHubsAndZonesCannotJoin();

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
    QVERIFY(lead->syncStatus().namesPrimary());   // naming a primary does not make a secondary
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
    m_house->player(0)->delay(QStringLiteral("/AddSlave"), 1000);
    lead->addSecondaries({ m_house->id(1), m_house->id(2) });
    QTRY_COMPARE(m_house->player(0)->requests(QStringLiteral("/AddSlave")).size(), 1);
    QTest::qWait(300);   // the first is not answered yet: no second request
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
    QCOMPARE(requests.at(0).target, QByteArray("/Volume?level=40&tell_slaves=1"));
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

static PlayerModel *household(FakeHousehold *house, QObject *parent)
{
    auto *model = new PlayerModel(parent);
    model->setClientTiming(fast());
    for (int i = 0; i < house->size(); ++i)
        model->addManual(house->id(i));
    return model;
}

void TstGroup::theCurrentPlayerFollowsItsPrimary()
{
    QScopedPointer<PlayerModel> model(household(m_house, nullptr));
    model->setCurrentId(m_house->id(1));
    QTRY_VERIFY(model->current()->online());
    // Kitchen joins Living Room's group from elsewhere.
    model->setPollAll(true);
    m_house->group(0, { 1 });
    QTRY_COMPARE(model->currentId(), m_house->id(0));
}

void TstGroup::choosingAMemberChoosesItsPrimary()
{
    m_house->group(0, { 1, 2 });
    QScopedPointer<PlayerModel> model(household(m_house, nullptr));
    model->setPollAll(true);
    QTRY_COMPARE(model->leaderOf(m_house->id(2)), m_house->id(0));
    model->setCurrentId(m_house->id(2));
    QCOMPARE(model->currentId(), m_house->id(0));
    QCOMPARE(model->memberIds(m_house->id(0)), QStringList({ m_house->id(1), m_house->id(2) }));
    const int row = model->indexOf(m_house->id(0));
    QCOMPARE(model->data(model->index(row), PlayerModel::MemberNamesRole).toStringList(),
             QStringList({ QStringLiteral("Kitchen"), QStringLiteral("Studio") }));
    QCOMPARE(model->data(model->index(model->indexOf(m_house->id(1))), PlayerModel::LeaderIdRole).toString(),
             m_house->id(0));
}

void TstGroup::watchingTheGroupKeepsMembersCurrent()
{
    m_house->group(0, { 1 });
    QScopedPointer<PlayerModel> model(household(m_house, nullptr));
    model->setCurrentId(m_house->id(0));
    QTRY_COMPARE(model->current()->groupRole(), Client::Primary);
    // Without the watch only the current player polls.
    QTest::qWait(500);
    QVERIFY(!model->client(m_house->id(1))->running());
    model->setWatchGroup(true);
    QTRY_VERIFY(model->client(m_house->id(1))->running());
    m_house->setVolume(1, 64);
    QTRY_COMPARE(model->client(m_house->id(1))->volume(), 64);
    QVERIFY(!model->client(m_house->id(2))->running());   // not a member
    model->setWatchGroup(false);
    QVERIFY(!model->client(m_house->id(1))->running());
    QVERIFY(!model->client(m_house->id(2))->running());
}

void TstGroup::joinAndLeaveThroughTheModel()
{
    QScopedPointer<PlayerModel> model(household(m_house, nullptr));
    model->setCurrentId(m_house->id(0));
    QTRY_VERIFY(model->current()->online());
    model->join(m_house->id(2));
    QTRY_COMPARE(m_house->leaderOf(2), 0);
    QTRY_COMPARE(model->current()->groupRole(), Client::Primary);
    model->leave(m_house->id(2));
    QTRY_COMPARE(m_house->leaderOf(2), -1);
    QTRY_COMPARE(model->current()->groupRole(), Client::Alone);
}

void TstGroup::aStaleGroupLoopDoesNotRecurse()
{
    // Kitchen last saw Living Room as its primary, then stopped polling.
    m_house->group(0, { 1 });
    QScopedPointer<PlayerModel> model(household(m_house, nullptr));
    model->setPollAll(true);
    model->setCurrentId(m_house->id(0));
    QTRY_COMPARE(model->client(m_house->id(1))->groupRole(), Client::Secondary);
    model->setPollAll(false);
    QVERIFY(!model->client(m_house->id(1))->running());
    // Elsewhere, Living Room now follows Kitchen, while Kitchen's last known
    // state still says it follows Living Room: a loop through stale states.
    m_house->group(1, { 0 });
    QTRY_COMPARE(model->currentId(), m_house->id(1));   // one step, no recursion
    QTest::qWait(500);
    QVERIFY(!model->currentId().isEmpty());
}

void TstGroup::thePlayerListShowsWhoCanJoin()
{
    // The player list polls everyone (pollAll); canJoin follows from that.
    m_house->group(1, { 2 });
    QScopedPointer<PlayerModel> model(household(m_house, nullptr));
    model->setCurrentId(m_house->id(0));
    model->setPollAll(true);
    auto canJoin = [&model](const QString &id) {
        return model->data(model->index(model->indexOf(id)), PlayerModel::CanJoinRole).toBool();
    };
    QTRY_COMPARE(model->client(m_house->id(2))->groupRole(), Client::Secondary);
    QTRY_COMPARE(model->client(m_house->id(1))->groupRole(), Client::Primary);
    QVERIFY(!canJoin(m_house->id(0)));   // the current player
    QVERIFY(!canJoin(m_house->id(1)));   // leads its own group
    QVERIFY(!canJoin(m_house->id(2)));   // in a group
    model->leave(m_house->id(2));        // through its own primary, not the current player
    QTRY_COMPARE(m_house->leaderOf(2), -1);
    QTRY_VERIFY(canJoin(m_house->id(2)));
    QCOMPARE(m_house->player(0)->requests(QStringLiteral("/RemoveSlave")).size(), 0);
}

void TstGroup::aMembershipChangeElsewhereShows()
{
    QScopedPointer<PlayerModel> model(household(m_house, nullptr));
    model->setCurrentId(m_house->id(0));
    QTRY_VERIFY(model->current()->online());
    model->setWatchGroup(true);
    m_house->group(0, { 2 });   // the official app groups Studio
    QTRY_COMPARE(model->memberIds(m_house->id(0)), QStringList({ m_house->id(2) }));
    QTRY_COMPARE(model->client(m_house->id(2))->groupRole(), Client::Secondary);
}

void TstGroup::aLonePlayersRequestsAreUnchanged()
{
    Client *lone = client(0);
    QTRY_VERIFY(lone->online());
    QTest::qWait(2500);
    QSet<QString> paths;
    for (const auto &r : m_house->player(0)->requests())
        paths.insert(r.path);
    const QSet<QString> expected{ QStringLiteral("/Status"), QStringLiteral("/SyncStatus"),
                                  QStringLiteral("/Presets"), QStringLiteral("/RadioBrowse"),
                                  QStringLiteral("/Playlist") };
    const QSet<QString> extra = paths - expected;
    QVERIFY2(extra.isEmpty(), qPrintable(QStringList(extra.values()).join(QLatin1Char(','))));
    // As before 005: one read at start, before /Status gives a syncStat, and
    // one more for the syncStat it then learns. No long poll.
    QCOMPARE(m_house->player(0)->requests(QStringLiteral("/SyncStatus")).size(), 2);
}

void TstGroup::aReconnectingSecondary()
{
    m_house->group(0, { 1 });
    m_house->setReconnecting(1, true);
    Client *member = client(1);
    QTRY_VERIFY(member->primaryReconnecting());
    m_house->setReconnecting(1, false);
    member->setSyncWatch(true);
    QTRY_VERIFY(!member->primaryReconnecting());
}

// An LSDP announcement (API appendix) for a household player on 127.0.0.1.
static QByteArray announce(char node, quint16 classId, quint16 port)
{
    auto prefixed = [](const QByteArray &b) { return char(b.size()) + b; };
    QByteArray m("A");
    m.append(prefixed(QByteArray("\x02\x00\x00\x00\x00", 5) + node));
    m.append(prefixed(QByteArray("\x7F\x00\x00\x01", 4)));
    m.append(char(1));
    m.append(char(classId >> 8)).append(char(classId & 0xFF)).append(char(1));
    m.append(prefixed("port")).append(prefixed(QByteArray::number(port)));
    return QByteArray("\x06LSDP\x01", 6) + char(m.size() + 1) + m;
}

void TstGroup::pairHalvesHubsAndZonesCannotJoin()
{
    auto *house = new FakeHousehold(5, this);
    house->setFixedSecondary(2, true);
    PlayerModel model;
    model.setClientTiming(fast());
    model.addManual(house->id(0));
    model.addManual(house->id(1));
    model.addManual(house->id(2));
    model.discovery()->handleDatagram(announce('\x03', 0x0006, house->port(3)));   // pair half
    model.discovery()->handleDatagram(announce('\x04', 0x0008, house->port(4)));   // Hub
    QCOMPARE(model.rowCount(), 5);
    model.setCurrentId(house->id(0));
    model.setPollAll(true);
    auto canJoin = [&model](const QString &id) {
        return model.data(model.index(model.indexOf(id)), PlayerModel::CanJoinRole).toBool();
    };
    QTRY_VERIFY(canJoin(house->id(1)));
    QTRY_VERIFY(model.client(house->id(2))->syncStatus().fixedSecondary);
    QTRY_VERIFY(model.client(house->id(3))->online());
    QTRY_VERIFY(model.client(house->id(4))->online());
    QVERIFY(!canJoin(house->id(0)));   // the current player
    QVERIFY(!canJoin(house->id(2)));   // in a fixed group
    QVERIFY(!canJoin(house->id(3)));   // the second half of a stereo pair
    QVERIFY(!canJoin(house->id(4)));   // a Hub
    model.setPollAll(false);
    delete house;
}

void TstGroup::groupAndMemberMute()
{
    m_house->group(0, { 1, 2 });
    QScopedPointer<PlayerModel> model(household(m_house, nullptr));
    model->setCurrentId(m_house->id(0));
    QTRY_COMPARE(model->current()->groupRole(), Client::Primary);
    // Group mute and group volume in one burst: different keys, both sent.
    model->current()->setGroupMuted(true);
    model->current()->setGroupVolume(20);
    QTRY_COMPARE(m_house->player(0)->requests(QStringLiteral("/Volume")).size(), 2);
    QTRY_COMPARE(m_house->volume(2), 20);
    model->setWatchGroup(true);
    QTRY_VERIFY(model->client(m_house->id(1))->muted());
    QTRY_VERIFY(model->client(m_house->id(2))->muted());
    // One member alone.
    model->setPlayerMuted(m_house->id(1), false);
    QTRY_VERIFY(!model->client(m_house->id(1))->muted());
    QTest::qWait(300);
    QVERIFY(model->client(m_house->id(2))->muted());
    QVERIFY(model->current()->muted());
}

QTEST_GUILESS_MAIN(TstGroup)
#include "tst_group.moc"
