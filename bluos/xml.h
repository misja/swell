// SPDX-License-Identifier: MIT
// Parsers for the control API documents (BluOS Custom Integration API v1.7,
// sections 2, 4.8, 6.1 and 11.1). Unknown elements and attributes are ignored, as the API
// requires; malformed input yields ok == false and an error message.
#ifndef BLUOS_XML_H
#define BLUOS_XML_H

#include <QByteArray>
#include <QString>
#include <QList>
#include <QMetaType>
#include <QStringList>

namespace BluOS {

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
    QString prid;           // preset list id; changes when the presets change (section 6.1)
    QString pid;            // play queue id; changes when the queue is replaced (section 5.1)
    int song = -1;          // current queue position; where playback resumes while streaming
    int secs = -1;          // position at the time of the response
    int totlen = -1;
    int volume = -1;        // 0..100, -1 = fixed volume
    bool mute = false;
    int groupVolume = -1;   // on a group's primary (section 2.1); -1 otherwise
    bool hasStreamUrl = false; // queue not the source: no next/previous
    // Stream actions (section 4.8): the URL to call for skip/back, empty when
    // the stream does not offer it. Relevant only with hasStreamUrl.
    QString skipActionUrl;
    QString backActionUrl;

    bool isPlaying() const { return state == QLatin1String("play") || state == QLatin1String("stream"); }
};

// /SyncStatus (section 2.2). The API names group members master and slave
// (and zoneMaster, zoneSlave for fixed groups) in its XML and requests;
// Swell's own names are primary and secondary.
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
    // On a secondary player in a group. The section 2.2 example also shows a
    // primary naming itself here, so Client::groupRole() compares it with the
    // player's own address.
    QString primaryHost;
    int primaryPort = 0;
    bool primaryReconnecting = false;
    // On the primary player of a group: "host:port" entries.
    QStringList secondaries;
    // A fixed group (stereo pair, surround set), configured in the player;
    // out of scope for grouping (section 8).
    QString fixedGroup;
    bool fixedPrimary = false;
    bool fixedSecondary = false;

    // Not the same as being a secondary; see above.
    bool namesPrimary() const { return !primaryHost.isEmpty(); }
};

// The answer to /AddSlave (section 8.1, 8.2): the players that were added.
// A player that could not be added is only missing from it.
struct GroupAdd {
    bool ok = false;
    QString error;

    QStringList added;      // "host:port"
};

// The answer to /Volume (section 3): the player's level after the change.
struct VolumeReply {
    bool ok = false;
    QString error;

    int level = -1;
    bool mute = false;
};

// /Presets (section 6.1). Ids are not sequential; play a preset by its id.
struct Preset {
    int id = 0;
    QString name;
    QString image;          // may be relative and may redirect, or an absolute URL
    QString url;            // what the preset loads, e.g. "Capture:…?id=<input id>"
    // The capture input this preset selects, from the id= in its url; empty otherwise.
    QString inputId() const;

    bool operator==(const Preset &o) const
    {
        return id == o.id && name == o.name && image == o.image && url == o.url;
    }
    bool operator!=(const Preset &o) const { return !(*this == o); }
};

struct Presets {
    bool ok = false;
    QString error;

    QString prid;
    QList<Preset> items;
};

// /RadioBrowse?service=Capture (section 11.1). Only local hardware inputs,
// i.e. top-level items with an inputType; cloud entries and Hub categories
// are skipped.
struct Input {
    QString id;
    QString text;
    QString inputType;      // e.g. spdif, analog, arc, bluetooth
    QString image;
    QString url;            // as given: already percent-encoded, play with /Play?url=

    bool operator==(const Input &o) const
    {
        return id == o.id && text == o.text && inputType == o.inputType && image == o.image
                && url == o.url;
    }
    bool operator!=(const Input &o) const { return !(*this == o); }
};

struct Inputs {
    bool ok = false;
    QString error;

    QList<Input> items;
};

// /Playlist (section 5.1): either the status form (length/id only) or a
// window of songs from ?start=&end=. A song's position is its queue index.
struct QueueEntry {
    int position = -1;
    QString title;
    QString artist;
    QString album;
    QString image;          // observed, not in 5.1; may be relative
    int duration = -1;      // seconds, from <time>; observed, not in 5.1
    QString songId;         // song attribute songid, when present
    QString fn;
    // Identifies the track independently of its position, for safe removal.
    QString identity() const { return (songId.isEmpty() ? fn : songId) + QLatin1Char('|') + title; }
};

struct Playlist {
    bool ok = false;
    QString error;

    int length = -1;        // the whole queue, not the window
    QString id;             // equals /Status pid
    QList<QueueEntry> songs;
};

// /Browse (section 7.1, search 7.2). All values are attributes. Keys and
// URLs are kept exactly as received (after XML unescaping).
struct BrowseAction {
    QString type;           // e.g. add-now, addAll-last, favourite-add
    QString text;
    QString actionUrl;
};

struct BrowseItem {
    QString type;           // link, audio, album, artist, track, section, …
    QString text;
    QString text2;
    QString image;          // may be relative
    QString browseKey;
    QString playUrl;
    QString autoplayUrl;
    QString contextMenuKey;
    QString inputType;
    int duration = -1;      // seconds, tracks only
    QString category;       // title of the enclosing <category>, if any
    QList<BrowseAction> actions;  // inline context menu
};

struct Browse {
    bool ok = false;
    QString error;          // parse error, or the player's <error> message
    bool playerError = false;

    QString type;           // menu, sections, albums, tracks, …
    QString serviceName;
    QString serviceIcon;
    QString searchKey;
    QString nextKey;
    QString parentKey;
    QString playUrl;        // the whole list (e.g. an album)
    QList<BrowseItem> items;
};

Status parseStatus(const QByteArray &xml);
Browse parseBrowse(const QByteArray &xml);
Playlist parsePlaylist(const QByteArray &xml);
Presets parsePresets(const QByteArray &xml);
Inputs parseCaptureInputs(const QByteArray &xml);
SyncStatus parseSyncStatus(const QByteArray &xml);
GroupAdd parseGroupAdd(const QByteArray &xml);
VolumeReply parseVolume(const QByteArray &xml);

} // namespace BluOS

// A browse model's ActionsRole holds a QList<BluOS::BrowseAction>.
Q_DECLARE_METATYPE(BluOS::BrowseAction)

#endif
