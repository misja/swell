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
    // The player shown on Home, Now Playing and the cover.
    Q_PROPERTY(QString currentId READ currentId WRITE setCurrentId NOTIFY currentChanged)
    Q_PROPERTY(QString currentNodeId READ currentNodeId NOTIFY currentChanged)
    Q_PROPERTY(QObject *current READ current NOTIFY currentChanged)
    // When false only the current player polls; set it while all players are on screen.
    Q_PROPERTY(bool pollAll READ pollAll WRITE setPollAll NOTIFY pollAllChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,   // "host:port"
        NameRole,
        ModelNameRole,
        OnlineRole,
        AvailabilityRole,
        ManualRole,
        PlayingRole,
        NowPlayingRole
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
    QObject *current() const;
    bool pollAll() const { return m_pollAll; }
    void setPollAll(bool pollAll);
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
    QObject *client(const QString &id) const;

    static bool parseAddress(const QString &address, QString *host, int *port);

signals:
    void countChanged();
    void searchingChanged();
    void manualHostsChanged();
    void currentChanged();
    void pollAllChanged();

private:
    struct Entry {
        QString nodeId;   // empty if never announced
        bool manual = false;
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

    QNetworkAccessManager m_nam;
    Discovery m_discovery;
    Client::Timing m_timing;
    QVector<Entry> m_entries;
    QString m_currentId;
    bool m_pollAll = false;
};

} // namespace BluOS

#endif
