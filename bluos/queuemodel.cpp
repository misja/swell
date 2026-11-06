// SPDX-License-Identifier: MIT
#include "queuemodel.h"

#include "client.h"

#include <QTimer>

namespace BluOS {

QueueModel::QueueModel(Client *client)
    : QAbstractListModel(client)
    , m_client(client)
{
}

int QueueModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant QueueModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size()) {
        return QVariant();
    }
    const int row = index.row();
    const Row &r = m_rows.at(row);
    if (!r.loaded) {
        want(row / WindowSize);
    }
    switch (role) {
    case PositionRole:
        return row;
    case TitleRole:
        return r.entry.title;
    case ArtistRole:
        return r.entry.artist;
    case AlbumRole:
        return r.entry.album;
    case DurationRole:
        return r.entry.duration;
    case ImageRole:
        return r.image;
    case CurrentRole:
        return row == m_current;
    case LoadedRole:
        return r.loaded;
    case IdentityRole:
        return r.loaded ? r.entry.identity() : QString();
    }
    return QVariant();
}

QHash<int, QByteArray> QueueModel::roleNames() const
{
    return {
        { PositionRole, "position" },
        { TitleRole, "title" },
        { ArtistRole, "artist" },
        { AlbumRole, "album" },
        { DurationRole, "duration" },
        { ImageRole, "image" },
        { CurrentRole, "current" },
        { LoadedRole, "loaded" },
        { IdentityRole, "identity" },
    };
}

void QueueModel::setActive(bool active)
{
    if (m_active == active) {
        return;
    }
    // The queue id changes on every modification (section 5.1), so rows kept
    // while inactive stay valid until the status poll reports another id.
    m_active = active;
    emit activeChanged();
    // Also when the first read failed or was cut off while off screen.
    if (m_active && (m_stale || !m_known)) {
        reload();
    }
}

QString QueueModel::identityAt(int position) const
{
    if (position < 0 || position >= m_rows.size() || !m_rows.at(position).loaded) {
        return QString();
    }
    return m_rows.at(position).entry.identity();
}

void QueueModel::removeEntry(int position)
{
    if (position < 0 || position >= m_rows.size()) {
        return;
    }
    beginRemoveRows(QModelIndex(), position, position);
    m_rows.remove(position);
    endRemoveRows();
    // A window read sent before the delete has old positions: ignore it, and
    // ask again for what was on its way.
    const int again = m_inFlight >= 0 ? m_inFlight : m_wanted;
    ++m_generation;
    m_inFlight = -1;
    m_wanted = -1;
    m_failed.clear();
    emit loadingChanged();
    if (again >= 0 && again * WindowSize < m_rows.size()) {
        want(again);
    }
    if (m_current > position) {
        --m_current;
        emit currentChanged();
    }
    emit lengthChanged();
}

void QueueModel::onStatus(const QString &pid, int song, bool streaming)
{
    if (!m_holdPid && isChange(pid)) {
        m_current = song;
        m_streaming = streaming;
        if (m_active) {
            reload();
        } else {
            m_stale = true;
        }
        emit currentChanged();
        return;
    }
    // song counts in the numbering of its queue id: only the current one fits
    // the rows (the others are ours, before or after the delete settles).
    if (!pid.isEmpty() && pid != (m_pid.isEmpty() ? m_reloadPid : m_pid)) {
        return;
    }
    setCurrent(song, streaming);
}

bool QueueModel::isChange(const QString &pid) const
{
    // While a reload waits for its first reply, the id it started from is
    // not a change.
    return !pid.isEmpty() && (m_pid.isEmpty() ? pid != m_reloadPid : !knowsPid(pid));
}

void QueueModel::setCurrent(int song, bool streaming)
{
    if (song == m_current && streaming == m_streaming) {
        return;
    }
    const int old = m_current;
    m_current = song;
    m_streaming = streaming;
    for (int row : { old, song }) {
        if (row >= 0 && row < m_rows.size()) {
            emit dataChanged(index(row), index(row), { CurrentRole });
        }
    }
    emit currentChanged();
}

void QueueModel::applyStatus()
{
    const Status &status = m_client->status();
    if (!status.pid.isEmpty() && status.pid == m_pid) {
        setCurrent(status.song, status.hasStreamUrl);
    }
}

void QueueModel::reload()
{
    if (!m_active) {
        m_stale = true;
        return;
    }
    m_stale = false;
    m_lineage.clear();
    ++m_generation;  // replies of earlier requests are ignored
    // Taken from the first reply: the status poll may not have caught up.
    m_pid.clear();
    m_reloadPid = m_client->status().pid;
    m_failed.clear();
    m_wanted = -1;
    m_inFlight = -1;
    m_awaitingFirst = true;
    m_known = false;
    setLength(0);
    emit lengthChanged();
    request(m_current > 0 ? m_current / WindowSize : 0);
}

void QueueModel::resetTo(const QString &pid, int length)
{
    ++m_generation;
    m_pid = pid;
    m_lineage.clear();
    m_failed.clear();
    m_wanted = -1;
    m_inFlight = -1;
    m_stale = false;
    m_known = true;
    setLength(length);
    emit lengthChanged();
    emit loadingChanged();
    if (m_awaitingFirst) {
        m_awaitingFirst = false;
        emit reloaded();
    }
}

void QueueModel::setHoldPid(bool hold)
{
    m_holdPid = hold;
    if (hold) {
        return;
    }
    // A change the status poll reported meanwhile, if it was not ours.
    if (isChange(m_client->status().pid)) {
        reload();
    } else {
        applyStatus();
    }
}

void QueueModel::adopt(const QString &pid)
{
    if (pid.isEmpty() || pid == m_pid) {
        return;
    }
    m_lineage.append(m_pid);
    m_pid = pid;
    emit lengthChanged();
    applyStatus();
}

void QueueModel::setLength(int length)
{
    if (length == m_rows.size()) {
        return;
    }
    beginResetModel();
    m_rows = QVector<Row>(qMax(0, length));
    endResetModel();
    emit lengthChanged();
}

bool QueueModel::windowLoaded(int window) const
{
    const int first = window * WindowSize;
    const int last = qMin(m_rows.size(), first + WindowSize);
    for (int i = first; i < last; ++i) {
        if (!m_rows.at(i).loaded) {
            return false;
        }
    }
    return true;
}

void QueueModel::want(int window) const
{
    if (!m_active || window == m_inFlight || windowLoaded(window)) {
        return;
    }
    if (m_failed.contains(window)
            && m_failed.value(window).elapsed() < m_client->timing().queueRetryMs) {
        return;
    }
    if (m_inFlight >= 0) {
        m_wanted = window;  // only the latest wish is kept
        return;
    }
    // Called from data() while the view builds delegates: send it later.
    m_inFlight = window;
    QueueModel *self = const_cast<QueueModel *>(this);
    const int generation = m_generation;
    QTimer::singleShot(0, self, [self, generation, window]() {
        if (generation == self->m_generation && self->m_inFlight == window) {
            self->request(window);
        }
    });
}

void QueueModel::request(int window)
{
    if (!m_active) {
        m_inFlight = -1;
        emit loadingChanged();
        return;
    }
    m_inFlight = window;
    emit loadingChanged();
    const int generation = m_generation;
    m_client->fetchQueueWindow(window * WindowSize, window * WindowSize + WindowSize - 1,
                               [this, generation, window](const Playlist &playlist) {
        onWindow(generation, window, playlist);
    });
}

void QueueModel::onWindow(int generation, int window, const Playlist &playlist)
{
    if (generation != m_generation) {
        return;  // a reply from before a reset
    }
    m_inFlight = -1;
    if (!playlist.ok) {
        m_failed[window].start();
        if (!m_known) {
            // Nothing on screen can ask again: retry the first window itself.
            QTimer::singleShot(m_client->timing().queueRetryMs, this, [this, generation, window]() {
                if (generation == m_generation && m_active && !m_known && m_inFlight < 0) {
                    request(window);
                }
            });
        }
    } else if (!m_pid.isEmpty() && !playlist.id.isEmpty() && playlist.id != m_pid) {
        // Another queue already; the status poll's pid change resets us.
    } else {
        if (m_pid.isEmpty()) {
            m_pid = playlist.id;
        }
        m_failed.remove(window);
        const bool wasKnown = m_known;
        m_known = true;
        if (playlist.length != m_rows.size()) {
            setLength(playlist.length);
        } else if (!wasKnown) {
            emit lengthChanged();
        }
        int first = -1;
        int last = -1;
        for (const QueueEntry &e : playlist.songs) {
            if (e.position < 0 || e.position >= m_rows.size()) {
                continue;
            }
            Row &r = m_rows[e.position];
            r.loaded = true;
            r.entry = e;
            r.image = m_client->resolveImage(e.image);
            first = first < 0 ? e.position : qMin(first, e.position);
            last = qMax(last, e.position);
        }
        if (first >= 0) {
            emit dataChanged(index(first), index(last));
        }
        if (!windowLoaded(window)) {
            m_failed[window].start();  // rows missing from the reply: not again at once
        }
    }
    emit loadingChanged();
    if (m_awaitingFirst) {
        m_awaitingFirst = false;
        emit reloaded();
    }
    if (m_wanted >= 0) {
        const int next = m_wanted;
        m_wanted = -1;
        want(next);
    }
}

} // namespace BluOS
