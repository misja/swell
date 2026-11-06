// SPDX-License-Identifier: MIT
// Parsers for the control API documents (BluOS Custom Integration API v1.7,
// sections 2 and 3). Unknown elements and attributes are ignored, as the API
// requires; malformed input yields ok == false and an error message.
#ifndef BLUOS_XML_H
#define BLUOS_XML_H

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace bluos {

// /Status (section 2.1).
struct Status {
    bool ok = false;
    QString error;

    QString etag;
    QString state;          // play, pause, stop, stream, connecting, ...
    // Three-line and two-line now-playing text; the API requires these to be
    // used instead of album/artist/name.
    QString title1;
    QString title2;
    QString title3;
    QString twolineTitle1;
    QString twolineTitle2;
    QString image;          // may be relative and may redirect (/Artwork)
    QString syncStat;       // matches /SyncStatus syncStat; changes when it changes
    int secs = -1;          // position at the time of the response
    int totlen = -1;
    int volume = -1;        // 0..100, -1 = fixed volume
    bool mute = false;
    bool hasStreamUrl = false; // queue not the source: no next/previous
    bool canSeek = false;

    bool isPlaying() const { return state == QLatin1String("play") || state == QLatin1String("stream"); }
};

// /SyncStatus (section 2.2).
struct SyncStatus {
    bool ok = false;
    QString error;

    QString etag;
    QString id;             // "host:port"
    QString name;
    QString model;
    QString modelName;
    QString brand;
    QString mac;
    QString syncStat;
    QString group;
    int volume = -1;        // 0..100, -1 = fixed volume
    bool mute = false;
    bool initialized = true;
    // Present only on a secondary player in a group.
    QString masterHost;
    int masterPort = 0;
    bool masterReconnecting = false;
    // Present only on the primary player of a group: "host:port" entries.
    QStringList slaves;

    bool isSecondary() const { return !masterHost.isEmpty(); }
};

// /Volume (section 3).
struct Volume {
    bool ok = false;
    QString error;

    QString etag;
    int level = -1;
    double db = 0.0;
    bool mute = false;
};

Status parseStatus(const QByteArray &xml);
SyncStatus parseSyncStatus(const QByteArray &xml);
Volume parseVolume(const QByteArray &xml);

} // namespace bluos

#endif
