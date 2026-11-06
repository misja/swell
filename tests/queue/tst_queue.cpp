// SPDX-License-Identifier: MIT
// QueueModel and the client's queue commands against the in-process
// FakePlayer, with a synthetic queue that /Delete and /Clear really change.
#include <QtTest>
#include <QNetworkAccessManager>

#include "client.h"
#include "fakeplayer.h"
#include "queuemodel.h"

using namespace BluOS;

class TstQueue : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void loadsTheCurrentWindowFirst();
    void pidChangeResets();
    void lengthChangeResets();
    void inactiveLoadsNothingThenReloadsWhenStale();
    void failedReadIsRetried();
    void staleRemoveIsRefused();
    void twoRemovesRunOneAfterTheOther();
    void clearReplyResets();
    void playFromSendsId();

private:
    void startClient();
    void setStatus(int song);
    QString title(int row);

    QNetworkAccessManager *m_nam = nullptr;
    FakePlayer *m_player = nullptr;
    Client *m_client = nullptr;
    QStringList m_queue;   // the player's queue: titles by position
    int m_pid = 7;
    int m_etag = 0;
};

void TstQueue::init()
{
    m_nam = new QNetworkAccessManager(this);
    m_player = new FakePlayer(this);
    QVERIFY(m_player->listen());
    m_queue.clear();
    for (int i = 0; i < 300; ++i)
        m_queue << QStringLiteral("Track %1").arg(i);
    m_pid = 7;
    m_player->serve(QStringLiteral("/SyncStatus"), "<SyncStatus etag=\"s\" syncStat=\"s\" name=\"Den\" volume=\"20\"/>");
    m_player->handle(QStringLiteral("/Playlist"), [this](const QUrlQuery &q) {
        const int start = q.queryItemValue(QStringLiteral("start")).toInt();
        const int end = q.queryItemValue(QStringLiteral("end")).toInt();
        QByteArray xml = "<playlist length=\"" + QByteArray::number(m_queue.size()) + "\" id=\""
                + QByteArray::number(m_pid) + "\">";
        for (int i = start; i <= end && i < m_queue.size(); ++i) {
            xml += "<song id=\"" + QByteArray::number(i) + "\" songid=\"S:" + m_queue.at(i).toUtf8()
                    + "\"><title>" + m_queue.at(i).toUtf8() + "</title><art>Nobody</art><alb>Invented</alb>"
                      "<time>200</time></song>";
        }
        return xml + "</playlist>";
    });
    m_player->handle(QStringLiteral("/Delete"), [this](const QUrlQuery &q) {
        const int id = q.queryItemValue(QStringLiteral("id")).toInt();
        if (id >= 0 && id < m_queue.size())
            m_queue.removeAt(id);
        return "<deleted>" + QByteArray::number(id) + "</deleted>";
    });
    m_player->handle(QStringLiteral("/Clear"), [this](const QUrlQuery &) {
        m_queue.clear();
        ++m_pid;
        return "<playlist modified=\"0\" length=\"0\" id=\"" + QByteArray::number(m_pid) + "\"/>";
    });
    m_player->serve(QStringLiteral("/Play"), "<state>play</state>");
    setStatus(0);
}

void TstQueue::cleanup()
{
    delete m_client;
    m_client = nullptr;
    delete m_player;
    m_player = nullptr;
    delete m_nam;
    m_nam = nullptr;
}

void TstQueue::setStatus(int song)
{
    m_player->setStatus("<status etag=\"e" + QByteArray::number(++m_etag) + "\"><state>play</state><pid>"
                        + QByteArray::number(m_pid) + "</pid><song>" + QByteArray::number(song)
                        + "</song></status>");
}

void TstQueue::startClient()
{
    m_client = new Client(m_nam, QStringLiteral("127.0.0.1"), m_player->port());
    Client::Timing t;
    t.pollTimeoutSecs = 1;
    t.deadlineGraceMs = 2000;
    t.commandTimeoutMs = 3000;
    t.queueReadTimeoutMs = 3000;
    t.minIntervalMs = 200;
    t.backoffInitialMs = 200;
    t.backoffMaxMs = 400;
    m_client->setTiming(t);
    m_client->start();
    QTRY_VERIFY(m_client->online());
}

QString TstQueue::title(int row)
{
    QueueModel *q = m_client->queueModel();
    return q->data(q->index(row), QueueModel::TitleRole).toString();
}

void TstQueue::loadsTheCurrentWindowFirst()
{
    setStatus(170);
    startClient();
    QueueModel *q = m_client->queueModel();
    QTRY_COMPARE(q->currentIndex(), 170);
    q->setActive(true);
    QTRY_COMPARE(q->length(), 300);
    QTRY_COMPARE(title(170), QStringLiteral("Track 170"));
    const auto reads = m_player->requests(QStringLiteral("/Playlist"));
    QCOMPARE(reads.first().target, QByteArray("/Playlist?start=150&end=199"));
    QCOMPARE(q->data(q->index(170), QueueModel::CurrentRole).toBool(), true);
    // An unloaded row is a placeholder until its window arrives.
    QCOMPARE(q->data(q->index(10), QueueModel::LoadedRole).toBool(), false);
    QTRY_COMPARE(title(10), QStringLiteral("Track 10"));
}

void TstQueue::pidChangeResets()
{
    startClient();
    QueueModel *q = m_client->queueModel();
    q->setActive(true);
    QTRY_COMPARE(q->length(), 300);
    m_queue = QStringList({ QStringLiteral("New A"), QStringLiteral("New B") });
    ++m_pid;
    setStatus(0);
    QTRY_COMPARE(q->length(), 2);
    QCOMPARE(q->pid(), QString::number(m_pid));
    QTRY_COMPARE(title(1), QStringLiteral("New B"));
}

void TstQueue::lengthChangeResets()
{
    startClient();
    QueueModel *q = m_client->queueModel();
    q->setActive(true);
    QTRY_COMPARE(q->length(), 300);
    // The queue grows under the same id (another controller added tracks).
    m_queue << QStringLiteral("Extra");
    q->data(q->index(120), QueueModel::TitleRole);  // ask for another window
    QTRY_COMPARE(q->length(), 301);
}

void TstQueue::inactiveLoadsNothingThenReloadsWhenStale()
{
    startClient();
    QueueModel *q = m_client->queueModel();
    QTest::qWait(500);
    QCOMPARE(m_player->requests(QStringLiteral("/Playlist")).size(), 0);
    ++m_pid;
    setStatus(0);
    QTest::qWait(500);
    QCOMPARE(m_player->requests(QStringLiteral("/Playlist")).size(), 0);
    q->setActive(true);
    QTRY_COMPARE(q->length(), 300);
    QCOMPARE(q->pid(), QString::number(m_pid));
}

void TstQueue::failedReadIsRetried()
{
    m_player->failWith(QStringLiteral("/Playlist"), 503);
    startClient();
    QueueModel *q = m_client->queueModel();
    q->setActive(true);
    QTRY_VERIFY(m_player->requests(QStringLiteral("/Playlist")).size() >= 1);
    QTRY_VERIFY(!q->loading());
    QCOMPARE(q->length(), 0);
    QVERIFY(!q->known());   // not shown as an empty queue
    m_player->stopFailing(QStringLiteral("/Playlist"));
    // The first window is retried by itself.
    QTRY_COMPARE_WITH_TIMEOUT(q->length(), 300, 8000);
    QVERIFY(q->known());
}

void TstQueue::staleRemoveIsRefused()
{
    startClient();
    QueueModel *q = m_client->queueModel();
    q->setActive(true);
    QTRY_COMPARE(title(3), QStringLiteral("Track 3"));
    QSignalSpy failed(m_client, &Client::commandFailed);
    const QString identity = q->identityAt(3);
    m_client->removeQueueEntry(3, QStringLiteral("not-the-pid"), identity);
    m_client->removeQueueEntry(3, q->pid(), QStringLiteral("someone else|x"));
    QCOMPARE(failed.size(), 2);
    QTest::qWait(400);
    QCOMPARE(m_player->requests(QStringLiteral("/Delete")).size(), 0);
}

void TstQueue::twoRemovesRunOneAfterTheOther()
{
    startClient();
    QueueModel *q = m_client->queueModel();
    q->setActive(true);
    QTRY_COMPARE(title(3), QStringLiteral("Track 3"));
    // Two remorses fire close together for rows 2 and 3.
    m_client->removeQueueEntry(2, q->pid(), q->identityAt(2));
    m_client->removeQueueEntry(3, q->pid(), q->identityAt(3));
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Delete")).size(), 2);
    // Track 3 moved to position 2 after the first delete, so both go to id=2.
    const auto deletes = m_player->requests(QStringLiteral("/Delete"));
    QCOMPARE(deletes.at(0).target, QByteArray("/Delete?id=2"));
    QCOMPARE(deletes.at(1).target, QByteArray("/Delete?id=2"));
    QVERIFY(!m_queue.contains(QStringLiteral("Track 2")));
    QVERIFY(!m_queue.contains(QStringLiteral("Track 3")));
    QCOMPARE(m_queue.at(2), QStringLiteral("Track 4"));
    QTRY_COMPARE(q->length(), 298);
    QTRY_COMPARE(title(2), QStringLiteral("Track 4"));
}

void TstQueue::clearReplyResets()
{
    startClient();
    QueueModel *q = m_client->queueModel();
    q->setActive(true);
    QTRY_COMPARE(q->length(), 300);
    m_client->clearQueue();
    QTRY_COMPARE(q->length(), 0);
    QCOMPARE(q->pid(), QString::number(m_pid));
    QCOMPARE(m_player->requests(QStringLiteral("/Clear")).size(), 1);
}

void TstQueue::playFromSendsId()
{
    startClient();
    m_client->playQueueEntry(5);
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Play")).size(), 1);
    QCOMPARE(m_player->requests(QStringLiteral("/Play")).first().target, QByteArray("/Play?id=5"));
}

QTEST_GUILESS_MAIN(TstQueue)
#include "tst_queue.moc"
