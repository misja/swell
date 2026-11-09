// SPDX-License-Identifier: MIT
#include "playermodel.h"

#include <QHostAddress>
#include <QUrl>

namespace BluOS {

PlayerModel::PlayerModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&m_discovery, &Discovery::found, this, &PlayerModel::onFound);
    connect(&m_discovery, &Discovery::lost, this, &PlayerModel::onLost);
    connect(&m_discovery, &Discovery::searchingChanged, this, &PlayerModel::searchingChanged);
    connect(&m_discovery, &Discovery::error, this, [](const QString &message) {
        qWarning("LSDP discovery unavailable: %s", qPrintable(message));
    });
}

PlayerModel::~PlayerModel()
{
    // Every client, including ones awaiting deleteLater after an address change
    // or removal, goes before m_nam: their replies must not outlive it.
    qDeleteAll(findChildren<Client *>(QString(), Qt::FindDirectChildrenOnly));
}

int PlayerModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant PlayerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return QVariant();
    const Entry &e = m_entries.at(index.row());
    const Client *c = e.client;
    switch (role) {
    case IdRole:
        return c->id();
    case NameRole:
        if (!c->name().isEmpty())
            return c->name();
        return e.discoveredName.isEmpty() ? c->host() : e.discoveredName;
    case ModelNameRole:
        return c->modelName();
    case OnlineRole:
        return c->online();
    case AvailabilityRole:
        return int(c->availability());
    case ManualRole:
        return e.manual;
    case PlayingRole:
        return c->playing();
    case NowPlayingRole:
        return c->status().twolineTitle1.isEmpty() ? c->title1() : c->status().twolineTitle1;
    case GroupRoleRole:
        return int(c->groupRole());
    case LeaderIdRole:
        return leaderOf(c->id());
    case MemberNamesRole: {
        QStringList names;
        for (const QString &id : memberIds(c->id())) {
            const int row = indexOf(id);
            names.append(row >= 0 ? data(this->index(row), NameRole).toString() : id);
        }
        return names;
    }
    case VolumeRole:
        return c->volume();
    case CanJoinRole:
        // A lone, reachable player other than the current one. Not the
        // second half of a stereo pair or a Hub (LSDP classes 0x0006 and
        // 0x0008), nor a player in a fixed group (zoneSlave in /SyncStatus).
        return c->id() != m_currentId && c->online() && c->groupRole() == Client::Alone
                && current() && current()->online() && current()->groupRole() != Client::Secondary
                && e.classId != Lsdp::ClassPairSecondary && e.classId != Lsdp::ClassHub
                && !c->syncStatus().fixedSecondary;
    case MutedRole:
        return c->muted();
    case PrimaryIdRole:
        return c->primaryId();
    case MemberIdsRole:
        return memberIds(c->id());
    }
    return QVariant();
}

QHash<int, QByteArray> PlayerModel::roleNames() const
{
    return {
        { IdRole, "playerId" },
        { NameRole, "name" },
        { ModelNameRole, "modelName" },
        { OnlineRole, "online" },
        { AvailabilityRole, "availability" },
        { ManualRole, "manual" },
        { PlayingRole, "playing" },
        { NowPlayingRole, "nowPlaying" },
        { GroupRoleRole, "groupRole" },
        { LeaderIdRole, "leaderId" },
        { MemberNamesRole, "memberNames" },
        { VolumeRole, "volume" },
        { CanJoinRole, "canJoin" },
        { MutedRole, "muted" },
        { PrimaryIdRole, "primaryId" },
        { MemberIdsRole, "memberIds" },
    };
}

QStringList PlayerModel::manualHosts() const
{
    QStringList hosts;
    for (const Entry &e : m_entries)
        if (e.manual)
            hosts.append(e.client->id());
    return hosts;
}

bool PlayerModel::parseAddress(const QString &address, QString *host, int *port)
{
    const QString trimmed = address.trimmed();
    if (trimmed.isEmpty() || trimmed.contains(QLatin1Char('/')) || trimmed.contains(QLatin1Char(' ')))
        return false;
    const QUrl url(QStringLiteral("http://") + trimmed);
    if (!url.isValid() || url.host().isEmpty() || !url.path().isEmpty() || url.hasQuery()
            || !url.userInfo().isEmpty())
        return false;
    *host = url.host();
    *port = url.port(11000);
    return *port > 0 && *port < 65536;
}

bool PlayerModel::isValidAddress(const QString &address) const
{
    QString host;
    int port = 0;
    return parseAddress(address, &host, &port);
}

bool PlayerModel::addManual(const QString &address)
{
    QString host;
    int port = 0;
    if (!parseAddress(address, &host, &port))
        return false;
    const int existing = indexOf(QStringLiteral("%1:%2").arg(host).arg(port));
    if (existing >= 0) {
        if (!m_entries[existing].manual) {
            m_entries[existing].manual = true;
            emit dataChanged(index(existing), index(existing), { ManualRole });
            emit manualHostsChanged();
        }
        return true;
    }
    Entry e;
    e.manual = true;
    e.client = createClient(host, port);
    insert(e);
    emit manualHostsChanged();
    return true;
}

void PlayerModel::removeManual(const QString &id)
{
    const int row = indexOf(id);
    if (row < 0 || !m_entries.at(row).manual)
        return;
    if (m_entries.at(row).nodeId.isEmpty()) {
        removeAt(row);
    } else {
        // Still announced on the network: keep it as a discovered player.
        m_entries[row].manual = false;
        emit dataChanged(index(row), index(row), { ManualRole });
    }
    emit manualHostsChanged();
}

void PlayerModel::search()
{
    m_discovery.search();
}

int PlayerModel::indexOf(const QString &id) const
{
    for (int i = 0; i < m_entries.size(); ++i)
        if (m_entries.at(i).client->id() == id)
            return i;
    return -1;
}

QString PlayerModel::idAt(int row) const
{
    return row >= 0 && row < m_entries.size() ? m_entries.at(row).client->id() : QString();
}

QString PlayerModel::nameAt(int row) const
{
    return data(index(row), NameRole).toString();
}

Client *PlayerModel::client(const QString &id) const
{
    const int row = indexOf(id);
    return row < 0 ? nullptr : m_entries.at(row).client;
}

void PlayerModel::setCurrentId(const QString &id)
{
    // One level: BluOS groups are flat, and a loop through stale group
    // states must not recurse.
    const QString leader = leaderOf(id);
    const QString target = leader.isEmpty() ? id : leader;
    if (m_currentId == target)
        return;
    m_currentId = target;
    applyPolicy();
    emit currentChanged();
}

QString PlayerModel::currentNodeId() const
{
    const int row = indexOf(m_currentId);
    return row < 0 ? QString() : m_entries.at(row).nodeId;
}

Client *PlayerModel::current() const
{
    return client(m_currentId);
}

void PlayerModel::setPollAll(bool pollAll)
{
    if (m_pollAll == pollAll)
        return;
    m_pollAll = pollAll;
    applyPolicy();
    emit pollAllChanged();
}

void PlayerModel::restoreCurrent(const QString &id, const QString &nodeId)
{
    QString host;
    int port = 0;
    if (!parseAddress(id, &host, &port))
        return;
    const QString normalized = QStringLiteral("%1:%2").arg(host).arg(port);
    if (indexOf(normalized) < 0) {
        Entry e;
        e.nodeId = nodeId;
        e.client = createClient(host, port);
        insert(e);
    }
    setCurrentId(normalized);
    applyPolicy();
}

void PlayerModel::setWatchGroup(bool watch)
{
    if (m_watchGroup == watch)
        return;
    m_watchGroup = watch;
    applyPolicy();
    emit watchGroupChanged();
}

void PlayerModel::applyPolicy()
{
    const QStringList members = m_watchGroup ? memberIds(m_currentId) : QStringList();
    for (const Entry &e : m_entries) {
        const bool current = e.client->id() == m_currentId;
        // Members shown with their own volume keep only /SyncStatus current
        // (a secondary's /Status is its primary's copy anyway). The player
        // list polls everyone with pollAll.
        const bool status = m_pollAll || current;
        const bool watch = members.contains(e.client->id());
        if (!status && !watch) {
            e.client->stop();
            continue;
        }
        e.client->setStatusPolling(status);
        e.client->setSyncWatch(watch);
        // Firmware checks for the current player only (spec 006).
        e.client->setUpgradeChecks(current);
        e.client->start();
    }    // Running or not changes which members count (groupMuted).
    updateCurrentGroup();
}

void PlayerModel::updateCurrentGroup()
{
    const QStringList members = memberIds(m_currentId);
    const bool muted = groupMuted(m_currentId);
    if (members == m_currentMemberIds && muted == m_currentGroupMuted)
        return;
    m_currentMemberIds = members;
    m_currentGroupMuted = muted;
    emit currentGroupChanged();
}

QString PlayerModel::memberId(const QString &ref) const
{
    if (indexOf(ref) >= 0)
        return ref;
    if (ref.startsWith(QLatin1String("127.")))
        return QString();   // a player's own idea of itself, useless from here
    for (const Entry &e : m_entries)
        if (e.client->syncStatus().id == ref)
            return e.client->id();
    return QString();
}

QString PlayerModel::leaderOf(const QString &id) const
{
    // A stopped client's group state is stale.
    const Client *c = client(id);
    return c && c->running() && c->groupRole() == Client::Secondary ? memberId(c->primaryId()) : QString();
}

QStringList PlayerModel::memberIds(const QString &leaderId) const
{
    QStringList ids;
    const Client *c = client(leaderId);
    if (!c)
        return ids;
    for (const QString &ref : c->secondaryIds()) {
        const QString id = memberId(ref);
        ids.append(id.isEmpty() ? ref : id);
    }
    return ids;
}

QString PlayerModel::addressOf(const QString &id) const
{
    const Client *c = client(id);
    if (!c)
        return id;   // a member reference: already "IP:port"
    if (!QHostAddress(c->host()).isNull())
        return c->id();
    // Added by name: use the address the player reports, if it is usable.
    const QString reported = c->syncStatus().id;
    return reported.isEmpty() || reported.startsWith(QLatin1String("127.")) ? c->id() : reported;
}

void PlayerModel::join(const QString &id)
{
    if (Client *lead = current())
        lead->addSecondaries({ addressOf(id) });
}

void PlayerModel::leave(const QString &id, const QString &leaderId)
{
    // The member's own primary, which may not be the current player; the
    // current player only if it lists this member.
    QString leader = leaderId.isEmpty() ? leaderOf(id) : leaderId;
    if (leader.isEmpty() && memberIds(m_currentId).contains(id))
        leader = m_currentId;
    if (Client *lead = client(leader))
        lead->removeSecondaries({ addressOf(id) });
}

void PlayerModel::setPlayerVolume(const QString &id, int level)
{
    if (Client *c = client(id))
        c->setVolume(level);
}

bool PlayerModel::groupMuted(const QString &leaderId) const
{
    const Client *lead = client(leaderId);
    if (!lead || !lead->muted())
        return false;
    for (const QString &id : memberIds(leaderId)) {
        // A stopped member's state is stale: only the ones kept current count.
        const Client *c = client(id);
        if (c && c->running() && !c->muted())
            return false;
    }
    return true;
}

void PlayerModel::setPlayerMuted(const QString &id, bool muted)
{
    if (Client *c = client(id))
        c->setMuted(muted);
}

void PlayerModel::onFound(const Discovery::Found &player)
{
    const QString host = player.address.toString();
    // Same node and port: the same player, possibly at a new address.
    for (int i = 0; i < m_entries.size(); ++i) {
        Entry &e = m_entries[i];
        if (e.nodeId == player.nodeId && e.client->port() == player.port) {
            e.discoveredName = player.name;
            e.classId = player.classId;
            if (e.client->host() != host) {
                const bool wasCurrent = e.client->id() == m_currentId;
                e.client->deleteLater();
                e.client = createClient(host, player.port);
                if (wasCurrent)
                    m_currentId = e.client->id();
                applyPolicy();
                if (wasCurrent)
                    emit currentChanged();
            }
            emit dataChanged(index(i), index(i));
            return;
        }
    }
    // A manually entered address that is now also announced.
    const int row = indexOf(QStringLiteral("%1:%2").arg(host).arg(player.port));
    if (row >= 0) {
        m_entries[row].nodeId = player.nodeId;
        m_entries[row].discoveredName = player.name;
        m_entries[row].classId = player.classId;
        emit dataChanged(index(row), index(row));
        if (m_entries.at(row).client->id() == m_currentId)
            emit currentChanged();  // the node id is now known for persistence
        return;
    }
    Entry e;
    e.nodeId = player.nodeId;
    e.discoveredName = player.name;
    e.classId = player.classId;
    e.client = createClient(host, player.port);
    insert(e);
}

void PlayerModel::onLost(const QString &nodeId)
{
    for (int i = m_entries.size() - 1; i >= 0; --i) {
        if (m_entries.at(i).nodeId != nodeId)
            continue;
        // The current player stays; its own poll shows whether it is gone.
        if (m_entries.at(i).client->id() == m_currentId)
            continue;
        if (m_entries.at(i).manual)
            m_entries[i].nodeId.clear();
        else
            removeAt(i);
    }
}

Client *PlayerModel::newClient(QNetworkAccessManager *nam, const QString &host, int port,
                               QObject *parent)
{
    return new Client(nam, host, port, parent);
}

Client *PlayerModel::createClient(const QString &host, int port)
{
    Client *c = newClient(&m_nam, host, port, this);
    c->setTiming(m_timing);
    connect(c, &Client::availabilityChanged, this, &PlayerModel::clientChanged);
    connect(c, &Client::statusChanged, this, &PlayerModel::clientChanged);
    connect(c, &Client::syncStatusChanged, this, &PlayerModel::clientChanged);
    return c;
}

void PlayerModel::insert(const Entry &entry)
{
    beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size());
    m_entries.append(entry);
    endInsertRows();
    applyPolicy();
    emit countChanged();
}

void PlayerModel::removeAt(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    Client *c = m_entries.at(row).client;
    const bool wasCurrent = c->id() == m_currentId;
    m_entries.remove(row);
    endRemoveRows();
    c->deleteLater();
    if (wasCurrent) {
        m_currentId.clear();
        emit currentChanged();
    }
    emit countChanged();
}

void PlayerModel::clientChanged()
{
    Client *c = qobject_cast<Client *>(sender());
    // A player's group shows on other rows too (a primary's member names).
    if (!m_entries.isEmpty())
        emit dataChanged(index(0), index(m_entries.size() - 1));
    if (c && c->id() == m_currentId && c->groupRole() == Client::Secondary) {
        // The current player joined a group elsewhere: follow its primary.
        const QString leader = memberId(c->primaryId());
        if (!leader.isEmpty())
            setCurrentId(leader);
    } else if (m_watchGroup && c && c->id() == m_currentId) {
        applyPolicy();   // the members may have changed
    }
    updateCurrentGroup();
}

} // namespace BluOS
