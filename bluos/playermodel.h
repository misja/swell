// SPDX-License-Identifier: MIT
// The players known to the app: found by LSDP, or entered by address.
// Each has a BluOS::Client; all clients share one QNetworkAccessManager.
#ifndef BLUOS_PLAYERMODEL_H
#define BLUOS_PLAYERMODEL_H

#include <QAbstractListModel>
#include <QNetworkAccessManager>
#include <QStringList>
#include <QVector>

#include "client.h"
#include "discovery.h"

namespace BluOS {

class PlayerModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    Q_PROPERTY(QStringList manualHosts READ manualHosts NOTIFY manualHostsChanged)
    // The player the app works with; with pollAll false, only it polls.
    Q_PROPERTY(QString currentId READ currentId WRITE setCurrentId NOTIFY currentChanged)
    Q_PROPERTY(QString currentNodeId READ currentNodeId NOTIFY currentChanged)
    Q_PROPERTY(BluOS::Client *current READ current NOTIFY currentChanged)
    // When false only the current player polls; set it while all players are on screen.
    Q_PROPERTY(bool pollAll READ pollAll WRITE setPollAll NOTIFY pollAllChanged)
    // Set while the current player's members are shown with their own volume
    // (Now Playing): they keep /SyncStatus current with a long poll.
    Q_PROPERTY(bool watchGroup READ watchGroup WRITE setWatchGroup NOTIFY watchGroupChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,   // "host:port"
        NameRole,
        ModelNameRole,
        OnlineRole,
        AvailabilityRole,
        ManualRole,
        PlayingRole,
        NowPlayingRole,
        GroupRoleRole,      // Client::GroupRole
        LeaderIdRole,       // the primary's id while a secondary it knows; else empty
        MemberNamesRole,    // a primary's secondaries, by name
        VolumeRole,         // the player's own volume, -1 if fixed or unknown
        CanJoinRole,        // can be added to the current player's group
        MutedRole,          // the player's own mute
        PrimaryIdRole       // a secondary's primary as it reports it, also when not in the model
    };

    explicit PlayerModel(QObject *parent = nullptr);
    ~PlayerModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool searching() const { return m_discovery.searching(); }
    QStringList manualHosts() const;
    Discovery *discovery() { return &m_discovery; }
    void setClientTiming(const Client::Timing &timing) { m_timing = timing; }

    QString currentId() const { return m_currentId; }
    void setCurrentId(const QString &id);
    QString currentNodeId() const;
    Client *current() const;
    bool pollAll() const { return m_pollAll; }
    void setPollAll(bool pollAll);
    bool watchGroup() const { return m_watchGroup; }
    void setWatchGroup(bool watch);

    // Groups (spec 005). A group is one player: making a secondary current
    // makes its primary current, and the current player follows its primary
    // when it joins a group elsewhere.
    // The model id for a member reference from /SyncStatus ("IP:port"): by
    // id, then by each player's own reported id. Empty if none matches.
    Q_INVOKABLE QString memberId(const QString &ref) const;
    // The primary's model id for a secondary, empty otherwise.
    Q_INVOKABLE QString leaderOf(const QString &id) const;
    // A primary's secondaries: model ids, or the reference when not known.
    Q_INVOKABLE QStringList memberIds(const QString &leaderId) const;
    // Adds a player to the current player's group.
    Q_INVOKABLE void join(const QString &id);
    // Takes a secondary out of its group, through its primary (leaderId when
    // given, for a member Swell does not know).
    Q_INVOKABLE void leave(const QString &id, const QString &leaderId = QString());
    // A player's own volume and mute (Now Playing, Per player).
    Q_INVOKABLE void setPlayerVolume(const QString &id, int level);
    Q_INVOKABLE void setPlayerMuted(const QString &id, bool muted);
    // Whether a primary and all its secondaries are muted.
    Q_INVOKABLE bool groupMuted(const QString &leaderId) const;
    // Makes a remembered player current at once, before discovery answers. With
    // its node id, a later announcement at a new address moves it there.
    Q_INVOKABLE void restoreCurrent(const QString &id, const QString &nodeId);

    // "host" or "host:port"; false if it is not a usable address.
    Q_INVOKABLE bool addManual(const QString &address);
    Q_INVOKABLE void removeManual(const QString &id);
    Q_INVOKABLE void search();
    Q_INVOKABLE int indexOf(const QString &id) const;
    Q_INVOKABLE bool isValidAddress(const QString &address) const;
    Q_INVOKABLE QString idAt(int row) const;
    Q_INVOKABLE QString nameAt(int row) const;
    Client *client(const QString &id) const;

    static bool parseAddress(const QString &address, QString *host, int *port);

signals:
    void countChanged();
    void searchingChanged();
    void manualHostsChanged();
    void currentChanged();
    void pollAllChanged();
    void watchGroupChanged();

protected:
    // Creates each player's client; a subclass may return a subclass of
    // Client. Not called from the constructor.
    virtual Client *newClient(QNetworkAccessManager *nam, const QString &host, int port,
                              QObject *parent);

private:
    struct Entry {
        QString nodeId;   // empty if never announced
        bool manual = false;
        quint16 classId = 0;   // LSDP class, 0 when added by address
        QString discoveredName;
        Client *client = nullptr;
    };

    void onFound(const Discovery::Found &player);
    void onLost(const QString &nodeId);
    Client *createClient(const QString &host, int port);
    void insert(const Entry &entry);
    void removeAt(int row);
    void clientChanged();
    void applyPolicy();
    // "IP:port" for /AddSlave and /RemoveSlave, which take IP addresses.
    QString addressOf(const QString &id) const;

    QNetworkAccessManager m_nam;
    Discovery m_discovery;
    Client::Timing m_timing;
    QVector<Entry> m_entries;
    QString m_currentId;
    bool m_pollAll = false;
    bool m_watchGroup = false;
};

} // namespace BluOS

#endif
