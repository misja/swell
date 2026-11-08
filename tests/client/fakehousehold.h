// SPDX-License-Identifier: MIT
// Several FakePlayers that can group (BluOS Custom Integration API v1.7,
// sections 2.2, 3 and 8), for tests only. The model follows azzurro's
// demo-player (jzbz/azzurro, MIT); no code is copied. Unlike that demo, each
// player has its own etag and syncStat, a secondary serves its primary's
// /Status, and /AddSlave, /RemoveSlave and /Volume take the forms with
// several players and tell_slaves. How real players remove a primary is not
// documented; this does what Home Assistant assumes (spec 005).
#ifndef FAKEHOUSEHOLD_H
#define FAKEHOUSEHOLD_H

#include <QList>
#include <QObject>
#include <QSet>
#include <QStringList>

#include "fakeplayer.h"

class FakeHousehold : public QObject
{
    Q_OBJECT

public:
    explicit FakeHousehold(int players, QObject *parent = nullptr) : QObject(parent)
    {
        for (int i = 0; i < players; ++i) {
            auto *player = new FakePlayer(this);
            player->listen();
            m_players.append(player);
            Room room;
            room.name = QStringLiteral("Room %1").arg(i + 1);
            room.status = "<state>stop</state>";
            m_rooms.append(room);
        }
        for (int i = 0; i < players; ++i)
            install(i);
        publish();
    }

    int size() const { return m_players.size(); }
    FakePlayer *player(int i) const { return m_players.at(i); }
    quint16 port(int i) const { return m_players.at(i)->port(); }
    QString id(int i) const { return QStringLiteral("127.0.0.1:%1").arg(port(i)); }
    int leaderOf(int i) const { return m_rooms.at(i).leader; }
    int volume(int i) const { return m_rooms.at(i).volume; }

    void setName(int i, const QString &name) { m_rooms[i].name = name; publish(); }
    // The inner XML of player i's <status>; members serve their leader's.
    void setStatus(int i, const QByteArray &inner) { m_rooms[i].status = inner; publish(); }
    void setVolume(int i, int level) { m_rooms[i].volume = level; publish(); }
    // Grouping as another controller would do it.
    void group(int leader, const QList<int> &members)
    {
        for (int m : members)
            if (m != leader)
                m_rooms[m].leader = leader;
        publish();
    }
    // An /AddSlave on any player silently leaves player i out (pyblu notes
    // that a failed add is only missing from the answer).
    void refuseToJoin(int i) { m_refused.insert(i); }
    // A secondary in a fixed group (stereo pair, surround set).
    void setZoneSlave(int i, bool on) { m_rooms[i].zoneSlave = on; publish(); }
    // A secondary that lost its primary and is trying to get it back.
    void setReconnecting(int i, bool on) { m_rooms[i].reconnecting = on; publish(); }
    // The §2.2 example: a primary also names itself in <master>.
    void setPrimaryNamesItself(bool on) { m_selfMaster = on; publish(); }

private:
    struct Room {
        QString name;
        int leader = -1;
        int volume = 30;
        bool mute = false;
        bool reconnecting = false;
        bool zoneSlave = false;
        QByteArray status;
        int syncGeneration = 0;
        int statusGeneration = 0;
        QByteArray lastSync;
        QByteArray lastStatus;
    };

    int indexOf(const QString &host, int port) const
    {
        for (int i = 0; i < m_players.size(); ++i)
            if ((host == QLatin1String("127.0.0.1") || host == QLatin1String("localhost"))
                    && this->port(i) == port)
                return i;
        return -1;
    }

    QList<int> membersOf(int leader) const
    {
        QList<int> members;
        for (int i = 0; i < m_rooms.size(); ++i)
            if (m_rooms.at(i).leader == leader)
                members.append(i);
        return members;
    }

    QByteArray syncBody(int i) const
    {
        const Room &r = m_rooms.at(i);
        QByteArray inner;
        const QList<int> members = membersOf(i);
        if (r.leader >= 0) {
            inner += "<master port=\"" + QByteArray::number(port(r.leader))
                    + "\" reconnecting=\"" + (r.reconnecting ? "true" : "false") + "\">127.0.0.1</master>";
        } else if (!members.isEmpty() && m_selfMaster) {
            inner += "<master port=\"" + QByteArray::number(port(i)) + "\">127.0.0.1</master>";
        }
        for (int m : members)
            inner += "<slave port=\"" + QByteArray::number(port(m)) + "\" id=\"127.0.0.1\"/>";
        QByteArray group;
        const int lead = r.leader >= 0 ? r.leader : (members.isEmpty() ? -1 : i);
        if (lead >= 0)
            group = m_rooms.at(lead).name.toUtf8() + " + " + QByteArray::number(membersOf(lead).size());
        return "name=\"" + r.name.toUtf8() + "\" volume=\"" + QByteArray::number(r.volume)
                + "\" mute=\"" + (r.mute ? "1" : "0") + "\" zoneSlave=\"" + (r.zoneSlave ? "true" : "false")
                + "\" group=\"" + group + "\" id=\""
                + id(i).toUtf8() + "\">" + inner;
    }

    // Recomputes every document; a player's etag moves only when its own
    // document changed.
    void publish()
    {
        for (int i = 0; i < m_rooms.size(); ++i) {
            Room &r = m_rooms[i];
            const QByteArray body = syncBody(i);
            if (body != r.lastSync) {
                r.lastSync = body;
                ++r.syncGeneration;
            }
        }
        for (int i = 0; i < m_rooms.size(); ++i) {
            Room &r = m_rooms[i];
            const QByteArray etag = "s" + QByteArray::number(i) + "-" + QByteArray::number(r.syncGeneration);
            m_players.at(i)->setDocument(QStringLiteral("/SyncStatus"),
                                         "<SyncStatus etag=\"" + etag + "\" syncStat=\"" + etag + "\" "
                                         + r.lastSync + "</SyncStatus>");
        }
        // A secondary's /Status is a copy of its primary's, syncStat included.
        for (int i = 0; i < m_rooms.size(); ++i) {
            Room &r = m_rooms[i];
            if (r.leader >= 0)
                continue;
            const QByteArray body = r.status + "<syncStat>s" + QByteArray::number(i) + "-"
                    + QByteArray::number(r.syncGeneration) + "</syncStat>";
            if (body != r.lastStatus) {
                r.lastStatus = body;
                ++r.statusGeneration;
            }
        }
        for (int i = 0; i < m_rooms.size(); ++i) {
            const int source = m_rooms.at(i).leader >= 0 ? m_rooms.at(i).leader : i;
            const Room &s = m_rooms.at(source);
            m_players.at(i)->setStatus("<status etag=\"p" + QByteArray::number(source) + "-"
                                       + QByteArray::number(s.statusGeneration) + "\">" + s.lastStatus
                                       + "</status>");
        }
    }

    static QStringList listOf(const QUrlQuery &q, const QString &one, const QString &many)
    {
        if (q.hasQueryItem(many))
            return q.queryItemValue(many).split(QLatin1Char(','));
        return QStringList(q.queryItemValue(one));
    }

    void install(int i)
    {
        FakePlayer *p = m_players.at(i);
        p->handle(QStringLiteral("/AddSlave"), [this, i](const QUrlQuery &q) -> QByteArray {
            const QStringList hosts = listOf(q, QStringLiteral("slave"), QStringLiteral("slaves"));
            const QStringList ports = listOf(q, QStringLiteral("port"), QStringLiteral("ports"));
            QByteArray added;
            for (int k = 0; k < hosts.size() && k < ports.size(); ++k) {
                const int j = indexOf(hosts.at(k), ports.at(k).toInt());
                // A leader, or the adder's own primary, cannot join: real
                // players are not expected to form loops (spec 005 review).
                if (j < 0 || j == i || m_refused.contains(j) || !membersOf(j).isEmpty()
                        || m_rooms.at(i).leader == j)
                    continue;
                m_rooms[j].leader = i;
                added += "<slave port=\"" + ports.at(k).toUtf8() + "\" id=\"" + hosts.at(k).toUtf8() + "\"/>";
            }
            publish();
            return "<addSlave>" + added + "</addSlave>";
        });
        p->handle(QStringLiteral("/RemoveSlave"), [this, i](const QUrlQuery &q) -> QByteArray {
            const QStringList hosts = listOf(q, QStringLiteral("slave"), QStringLiteral("slaves"));
            const QStringList ports = listOf(q, QStringLiteral("port"), QStringLiteral("ports"));
            for (int k = 0; k < hosts.size() && k < ports.size(); ++k) {
                const int j = indexOf(hosts.at(k), ports.at(k).toInt());
                if (j < 0)
                    continue;
                if (j == i) {
                    // The primary leaves: the others regroup under the first
                    // of them if two or more remain (inferred, see above).
                    const QList<int> rest = membersOf(i);
                    for (int m : rest)
                        m_rooms[m].leader = rest.size() >= 2 && m != rest.first() ? rest.first() : -1;
                } else if (m_rooms.at(j).leader == i) {
                    m_rooms[j].leader = -1;
                }
            }
            publish();
            const QByteArray etag = "s" + QByteArray::number(i) + "-"
                    + QByteArray::number(m_rooms.at(i).syncGeneration);
            return "<SyncStatus etag=\"" + etag + "\" syncStat=\"" + etag + "\" " + m_rooms.at(i).lastSync
                    + "</SyncStatus>";
        });
        p->handle(QStringLiteral("/Volume"), [this, i](const QUrlQuery &q) -> QByteArray {
            QList<int> targets{ i };
            if (q.queryItemValue(QStringLiteral("tell_slaves")) == QLatin1String("1"))
                targets += membersOf(i);
            for (int t : targets) {
                if (q.hasQueryItem(QStringLiteral("level")))
                    m_rooms[t].volume = qBound(0, q.queryItemValue(QStringLiteral("level")).toInt(), 100);
                if (q.hasQueryItem(QStringLiteral("mute")))
                    m_rooms[t].mute = q.queryItemValue(QStringLiteral("mute")) == QLatin1String("1");
            }
            publish();
            return "<volume db=\"-30\" mute=\"" + QByteArray(m_rooms.at(i).mute ? "1" : "0") + "\">"
                    + QByteArray::number(m_rooms.at(i).volume) + "</volume>";
        });
    }

    QList<FakePlayer *> m_players;
    QList<Room> m_rooms;
    QSet<int> m_refused;
    bool m_selfMaster = false;
};

#endif
