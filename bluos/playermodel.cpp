// SPDX-License-Identifier: MIT
#include "playermodel.h"

#include <QHostAddress>
#include <QUrl>

namespace bluos {

PlayerModel::PlayerModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&m_discovery, &Discovery::found, this, &PlayerModel::onFound);
    connect(&m_discovery, &Discovery::lost, this, &PlayerModel::onLost);
    connect(&m_discovery, &Discovery::searchingChanged, this, &PlayerModel::searchingChanged);
}

PlayerModel::~PlayerModel()
{
    for (const Entry &e : m_entries)
        delete e.client;
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
    case ClientRole:
        return QVariant::fromValue<QObject *>(e.client);
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
        { ClientRole, "client" },
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

QObject *PlayerModel::client(const QString &id) const
{
    const int row = indexOf(id);
    return row < 0 ? nullptr : m_entries.at(row).client;
}

void PlayerModel::onFound(const Discovery::Found &player)
{
    const QString host = player.address.toString();
    // Same node and port: the same player, possibly at a new address.
    for (int i = 0; i < m_entries.size(); ++i) {
        Entry &e = m_entries[i];
        if (e.nodeId == player.nodeId && e.client->port() == player.port) {
            e.discoveredName = player.name;
            if (e.client->host() != host) {
                e.client->deleteLater();
                e.client = createClient(host, player.port);
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
        emit dataChanged(index(row), index(row));
        return;
    }
    Entry e;
    e.nodeId = player.nodeId;
    e.discoveredName = player.name;
    e.client = createClient(host, player.port);
    insert(e);
}

void PlayerModel::onLost(const QString &nodeId)
{
    for (int i = m_entries.size() - 1; i >= 0; --i) {
        if (m_entries.at(i).nodeId != nodeId)
            continue;
        if (m_entries.at(i).manual)
            m_entries[i].nodeId.clear();
        else
            removeAt(i);
    }
}

Client *PlayerModel::createClient(const QString &host, int port)
{
    Client *c = new Client(&m_nam, host, port);
    c->setTiming(m_timing);
    connect(c, &Client::availabilityChanged, this, &PlayerModel::clientChanged);
    connect(c, &Client::statusChanged, this, &PlayerModel::clientChanged);
    connect(c, &Client::syncStatusChanged, this, &PlayerModel::clientChanged);
    c->start();
    return c;
}

void PlayerModel::insert(const Entry &entry)
{
    beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size());
    m_entries.append(entry);
    endInsertRows();
    emit countChanged();
}

void PlayerModel::removeAt(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    Client *c = m_entries.at(row).client;
    m_entries.remove(row);
    endRemoveRows();
    c->deleteLater();
    emit countChanged();
}

void PlayerModel::clientChanged()
{
    Client *c = qobject_cast<Client *>(sender());
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).client == c) {
            emit dataChanged(index(i), index(i));
            return;
        }
    }
}

} // namespace bluos
