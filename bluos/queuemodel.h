// SPDX-License-Identifier: MIT
// The play queue of one player (BluOS Custom Integration API v1.7, section 5)
// as a list model of fixed length. Rows not yet loaded are placeholders; the
// window holding a requested row is fetched with /Playlist?start=&end=, the
// window around the current entry first. Only one window is in flight.
#ifndef BLUOS_QUEUEMODEL_H
#define BLUOS_QUEUEMODEL_H

#include <QAbstractListModel>
#include <QElapsedTimer>
#include <QHash>
#include <QStringList>
#include <QUrl>
#include <QVector>

#include "xml.h"

namespace BluOS {

class Client;

class QueueModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int length READ length NOTIFY lengthChanged)
    Q_PROPERTY(QString pid READ pid NOTIFY lengthChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    // True once the length is known; an empty queue is only shown then.
    Q_PROPERTY(bool known READ known NOTIFY lengthChanged)
    // Set while the queue is on screen; an inactive model loads nothing.
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentChanged)
    // True while an input or stream plays: the current entry is only where
    // playback resumes (section 2.1).
    Q_PROPERTY(bool streaming READ streaming NOTIFY currentChanged)

public:
    enum Roles {
        PositionRole = Qt::UserRole + 1,
        TitleRole,
        ArtistRole,
        AlbumRole,
        DurationRole,
        ImageRole,
        CurrentRole,
        LoadedRole,
        IdentityRole
    };

    static const int WindowSize = 50;

    explicit QueueModel(Client *client);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int length() const { return m_rows.size(); }
    QString pid() const { return m_pid; }
    bool loading() const { return m_inFlight >= 0; }
    bool known() const { return m_known; }
    bool active() const { return m_active; }
    void setActive(bool active);
    int currentIndex() const { return m_current; }
    bool streaming() const { return m_streaming; }

    QString identityAt(int position) const;
    // Changes whenever the rows are replaced or shifted.
    int generation() const { return m_generation; }
    // The player removed this entry: drop the row, keep the rest.
    void removeEntry(int position);
    // While Swell's own delete settles, a pid change is not a new queue.
    void setHoldPid(bool hold);
    // Swell's own delete gave the queue this id; the rows already match.
    void adopt(const QString &pid);
    // The current id, or one this queue had before Swell's own deletes.
    bool knowsPid(const QString &pid) const { return pid == m_pid || m_lineage.contains(pid); }

    // From the client's status poll.
    void onStatus(const QString &pid, int song, bool streaming);
    // Drops everything and loads again, starting at the current window.
    void reload();
    // The queue is now this one (e.g. the reply of /Clear).
    void resetTo(const QString &pid, int length);

signals:
    void lengthChanged();
    void loadingChanged();
    void activeChanged();
    void currentChanged();
    // The first window after a reload has arrived (or failed).
    void reloaded();

private:
    struct Row {
        bool loaded = false;
        QueueEntry entry;
        QUrl image;
    };

    void want(int window) const;
    void request(int window);
    void onWindow(int generation, int window, const Playlist &playlist);
    bool windowLoaded(int window) const;
    void setLength(int length);

    Client *m_client;
    QVector<Row> m_rows;
    QString m_pid;
    QStringList m_lineage;   // earlier ids, changed only by Swell's own deletes
    bool m_holdPid = false;
    int m_current = -1;
    bool m_streaming = false;
    bool m_active = false;
    bool m_stale = true;
    bool m_awaitingFirst = false;
    bool m_known = false;
    int m_generation = 0;
    mutable int m_inFlight = -1;
    mutable int m_wanted = -1;
    mutable QHash<int, QElapsedTimer> m_failed;
};

} // namespace BluOS

#endif
