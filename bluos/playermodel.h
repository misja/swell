// SPDX-License-Identifier: MIT
// The players known to the app: found by LSDP, or entered by address.
// Each has a bluos::Client; all clients share one QNetworkAccessManager.
#ifndef BLUOS_PLAYERMODEL_H
#define BLUOS_PLAYERMODEL_H

#include <QAbstractListModel>
#include <QNetworkAccessManager>
#include <QStringList>
#include <QVector>

#include "client.h"
#include "discovery.h"

namespace bluos {

class PlayerModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    Q_PROPERTY(QStringList manualHosts READ manualHosts NOTIFY manualHostsChanged)

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
        ClientRole
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

    // "host" or "host:port"; false if it is not a usable address.
    Q_INVOKABLE bool addManual(const QString &address);
    Q_INVOKABLE void removeManual(const QString &id);
    Q_INVOKABLE void search();
    Q_INVOKABLE int indexOf(const QString &id) const;
    Q_INVOKABLE QObject *client(const QString &id) const;

    static bool parseAddress(const QString &address, QString *host, int *port);

signals:
    void countChanged();
    void searchingChanged();
    void manualHostsChanged();

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

    QNetworkAccessManager m_nam;
    Discovery m_discovery;
    Client::Timing m_timing;
    QVector<Entry> m_entries;
};

} // namespace bluos

#endif
