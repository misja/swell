// SPDX-License-Identifier: MIT
// One BluOS player over the control API (BluOS Custom Integration API v1.7):
// a /Status long poll, /SyncStatus refreshed when its syncStat changes, and
// transport and volume commands. Usable from C++ and, through its properties,
// from QML.
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
#include <QVariantList>

#include "xml.h"

class QNetworkAccessManager;
class QNetworkReply;

namespace bluos {

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
    Q_PROPERTY(int duration READ duration NOTIFY statusChanged)
    Q_PROPERTY(int position READ position NOTIFY positionChanged)
    Q_PROPERTY(QVariantList presets READ presets NOTIFY presetsChanged)
    Q_PROPERTY(QVariantList inputs READ inputs NOTIFY inputsChanged)

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
    bool canSkip() const { return m_status.ok && !m_status.hasStreamUrl; }
    int duration() const { return m_status.totlen; }
    int position() const;
    // Each entry: id (int), name, image (resolved URL), inputId (the capture
    // input it selects, or empty). Cached per player.
    QVariantList presets() const { return m_presets; }
    // Each entry: id, text, inputType, image (resolved URL), url (encoded, as given).
    QVariantList inputs() const { return m_inputs; }

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

signals:
    void availabilityChanged();
    void statusChanged();
    void syncStatusChanged();
    void positionChanged();
    void presetsChanged();
    void inputsChanged();
    // A command request failed; the poll keeps running.
    void commandFailed(const QString &path, const QString &error);

private:
    struct Gate {
        QElapsedTimer lastSent;
        QTimer *timer = nullptr;
        QString pending;   // the query, already percent-encoded
        bool hasPending = false;
    };

    static QString encoded(const QUrlQuery &query);
    QUrl url(const QString &path, const QString &encodedQuery = QString()) const;
    void send(const QString &path, const QString &encodedQuery);
    void dispatch(const QString &path);
    int msUntilAllowed(const QString &path);
    QNetworkReply *get(const QString &path, const QString &encodedQuery, int deadlineMs);
    // One GET under the per-path interval; the handler gets the body on success.
    void fetch(const QString &path, const QString &encodedQuery,
               const std::function<void(const QByteArray &)> &handler);
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
    QHash<QString, Gate> m_gates;
    QVariantList m_presets;
    QVariantList m_inputs;
    QString m_presetsPrid;
    bool m_presetsLoading = false;
    bool m_inputsLoading = false;
    bool m_presetsFetched = false;
    bool m_inputsFetched = false;
};

} // namespace bluos

#endif
