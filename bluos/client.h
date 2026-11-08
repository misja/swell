// SPDX-License-Identifier: MIT
// One BluOS player over the control API (BluOS Custom Integration API v1.7):
// a /Status long poll, /SyncStatus refreshed when its syncStat changes, and
// transport and volume commands.
#ifndef BLUOS_CLIENT_H
#define BLUOS_CLIENT_H

#include <functional>

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include "xml.h"
#include "queuemodel.h"

class QNetworkAccessManager;
class QNetworkReply;

namespace BluOS {

class Client : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString host READ host CONSTANT)
    Q_PROPERTY(int port READ port CONSTANT)
    Q_PROPERTY(Availability availability READ availability NOTIFY availabilityChanged)
    Q_PROPERTY(bool online READ online NOTIFY availabilityChanged)
    Q_PROPERTY(QString name READ name NOTIFY syncStatusChanged)
    Q_PROPERTY(QString modelName READ modelName NOTIFY syncStatusChanged)
    Q_PROPERTY(int volume READ volume NOTIFY syncStatusChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY syncStatusChanged)
    Q_PROPERTY(bool fixedVolume READ fixedVolume NOTIFY syncStatusChanged)
    Q_PROPERTY(QString state READ state NOTIFY statusChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY statusChanged)
    Q_PROPERTY(QString title1 READ title1 NOTIFY statusChanged)
    Q_PROPERTY(QString title2 READ title2 NOTIFY statusChanged)
    Q_PROPERTY(QString title3 READ title3 NOTIFY statusChanged)
    Q_PROPERTY(QUrl artworkUrl READ artworkUrl NOTIFY statusChanged)
    Q_PROPERTY(bool canSkip READ canSkip NOTIFY statusChanged)
    Q_PROPERTY(bool canBack READ canBack NOTIFY statusChanged)
    Q_PROPERTY(int duration READ duration NOTIFY statusChanged)
    Q_PROPERTY(int position READ position NOTIFY positionChanged)
    Q_PROPERTY(BluOS::QueueModel *queue READ queue CONSTANT)

public:
    enum Availability {
        Connecting,   // no answer yet
        Online,
        Offline,      // was reachable, or could not be reached; retrying
        NotAPlayer    // answered, but not with BluOS documents; retrying slowly
    };
    Q_ENUM(Availability)

    struct Timing {
        int pollTimeoutSecs = 100;   // API 2.1: recommended 100 s, never below 10 s
        int deadlineGraceMs = 15000; // added to the poll timeout before aborting
        int commandTimeoutMs = 10000;
        int readTimeoutMs = 30000;  // queue and browse reads; the player answers after serving covers
        int queueRetryMs = 3000;         // pause before a failed window is read again
        int minIntervalMs = 1000;    // API 2: same resource at most once per second
        int backoffInitialMs = 1000;
        int backoffMaxMs = 30000;
    };

    Client(QNetworkAccessManager *nam, const QString &host, int port = 11000,
           QObject *parent = nullptr);
    ~Client() override;

    void setTiming(const Timing &timing);  // before start(); tests use short values
    Timing timing() const { return m_timing; }

    QString host() const { return m_host; }
    int port() const { return m_port; }
    QString id() const;                    // "host:port"

    bool running() const { return m_running; }
    Availability availability() const { return m_availability; }
    bool online() const { return m_availability == Online; }
    const Status &status() const { return m_status; }
    const SyncStatus &syncStatus() const { return m_sync; }

    QString name() const { return m_sync.name; }
    QString modelName() const { return m_sync.modelName; }
    int volume() const { return m_sync.volume; }
    bool muted() const { return m_sync.mute; }
    bool fixedVolume() const { return m_sync.ok && m_sync.volume < 0; }
    QString state() const { return m_status.state; }
    bool playing() const { return m_status.isPlaying(); }
    QString title1() const { return m_status.title1; }
    QString title2() const { return m_status.title2; }
    QString title3() const { return m_status.title3; }
    QUrl artworkUrl() const { return resolveImage(m_status.image); }
    // The queue can always skip; a stream only when it offers the action (4.8).
    bool canSkip() const { return m_status.ok && (!m_status.hasStreamUrl || !m_status.skipActionUrl.isEmpty()); }
    bool canBack() const { return m_status.ok && (!m_status.hasStreamUrl || !m_status.backActionUrl.isEmpty()); }
    int duration() const { return m_status.totlen; }
    int position() const;
    // As the player lists them; images as given, see resolveImage. Cached per
    // player.
    QList<Preset> presets() const { return m_presets; }
    QList<Input> inputs() const { return m_inputs; }
    QueueModel *queue() const { return m_queue; }

    // One /Playlist window (section 5.1); the handler gets ok == false on failure.
    void fetchQueueWindow(int start, int end, const std::function<void(const Playlist &)> &handler);
    // One /Browse read (sections 7.1, 7.2): the top level for an empty key, a
    // search when q is set. Keys are sent byte for byte (encoded once). Not
    // paced by the one-second rule, which the API sets for status polling.
    void fetchBrowse(const QString &key, const QString &q, const std::function<void(const Browse &)> &handler);
    // Splits a player path from a /Browse reply into path and encoded query.
    // False unless it starts with a single '/', has no fragment and survives
    // Qt's URL handling unchanged; the host is always this player.
    bool splitPlayerPath(const QString &playerPath, QString *path, QString *query) const;

    // Resolves an image path from a player document against this player and
    // adds followRedirects=1 to player-served images (API v1.7, image attributes).
    QUrl resolveImage(const QString &image) const;

public slots:
    void start();
    void stop();

    void play();
    void pause();
    void skip();
    void back();
    void setVolume(int level);
    void setMuted(bool muted);
    void playPreset(int id);
    // Plays a capture input by the URL attribute from /RadioBrowse?service=Capture,
    // sent byte for byte. A URL that would not form a valid query is refused.
    void playInput(const QString &encodedUrl);
    void refreshInputs();
    void playQueueEntry(int position);
    // Removes the entry at position, but only if the player confirms that the
    // queue is still pid and the entry there is still identity. Removals run
    // one at a time; later ones move up as earlier ones succeed (section 5.2).
    void removeQueueEntry(int position, const QString &pid, const QString &identity);
    void clearQueue();
    // Plays a playURL or autoplayURL from /Browse; the last tap wins.
    void playPath(const QString &playerPath);
    // Runs a context-menu actionURL (add, add all, favourite). Never merged
    // with another request; actionDone or commandFailed follows.
    void runAction(const QString &playerPath, const QString &type);

signals:
    void availabilityChanged();
    void statusChanged();
    void syncStatusChanged();
    void positionChanged();
    void presetsChanged();
    void inputsChanged();
    // A command request failed; the poll keeps running.
    void commandFailed(const QString &path, const QString &error);
    // A removal was not sent because the queue no longer matched.
    void removeRefused();
    // A context-menu action succeeded (type as in the menu, e.g. addAll-last).
    void actionDone(const QString &type);

private:
    struct Gate {
        QString path;
        QTimer *timer = nullptr;
        QString pending;   // the query, already percent-encoded
        bool hasPending = false;
    };

    static QString encoded(const QUrlQuery &query);
    QUrl url(const QString &path, const QString &encodedQuery = QString()) const;
    void send(const QString &path, const QString &encodedQuery);
    void dispatch(const QString &path);
    int msUntilAllowed(const QString &path) const;
    QNetworkReply *get(const QString &path, const QString &encodedQuery, int deadlineMs);
    // Sends a player-given action URL ("/Action?service=…&skip=…") as given.
    void sendAction(const QString &actionUrl);
    // One GET under the per-path interval; the handler gets the body on success.
    void fetch(const QString &path, const QString &encodedQuery,
               const std::function<void(const QByteArray &)> &handler, int deadlineMs = -1,
               bool paced = true);
    void processRemovals();
    void adoptQueueId(int position);
    void finishRemoval();
    static QString errorIn(const QByteArray &body);
    void fetchPresets();

    void pollStatus();
    void onStatusReply(QNetworkReply *reply);
    void fetchSyncStatus();
    void onSyncStatusReply(QNetworkReply *reply);
    void onCommandReply(QNetworkReply *reply, const QString &path);

    void succeeded();
    void failed(Availability availability);
    void setAvailability(Availability availability);
    void schedulePoll(int delayMs);

    QNetworkAccessManager *m_nam;
    QString m_host;
    int m_port;
    Timing m_timing;
    bool m_running = false;

    Availability m_availability = Connecting;
    Status m_status;
    SyncStatus m_sync;
    QElapsedTimer m_statusAge;

    QPointer<QNetworkReply> m_statusReply;
    QPointer<QNetworkReply> m_syncReply;
    QTimer m_pollTimer;
    QTimer m_positionTimer;
    int m_backoffMs = 0;
    QHash<QString, Gate> m_gates;       // keyed by path and parameter name
    QHash<QString, QElapsedTimer> m_lastSent;  // keyed by path
    QString m_syncRequestedFor;
    QList<Preset> m_presets;
    QList<Input> m_inputs;
    QString m_presetsPrid;
    bool m_presetsLoading = false;
    bool m_inputsLoading = false;
    QueueModel *m_queue;
    struct Removal {
        int position;
        QString pid;
        QString identity;
    };
    QList<Removal> m_removals;
    bool m_removing = false;
    bool m_presetsFetched = false;
    bool m_inputsFetched = false;
};

} // namespace BluOS

#endif
