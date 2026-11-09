// SPDX-License-Identifier: MIT
#include "xml.h"

#include <QXmlStreamReader>

namespace BluOS {

namespace {

int toInt(const QString &text, int fallback)
{
    bool ok = false;
    const int value = text.trimmed().toInt(&ok);
    return ok ? value : fallback;
}

bool toBool(const QString &text)
{
    const QString t = text.trimmed();
    return t == QLatin1String("1") || t.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
}

// Moves the reader to the root element; false with error set if it is not `name`.
bool enterRoot(QXmlStreamReader &reader, const char *name, QString *error)
{
    if (!reader.readNextStartElement()) {
        *error = reader.hasError() ? reader.errorString() : QStringLiteral("empty document");
        return false;
    }
    if (reader.name() != QLatin1String(name)) {
        *error = QStringLiteral("unexpected root element <%1>").arg(reader.name().toString());
        return false;
    }
    return true;
}

// Skips everything up to the end of the document; reports malformed input.
bool finish(QXmlStreamReader &reader, QString *error)
{
    while (!reader.atEnd())
        reader.readNext();
    if (reader.hasError()) {
        *error = reader.errorString();
        return false;
    }
    return true;
}

} // namespace

Status parseStatus(const QByteArray &xml)
{
    Status s;
    QXmlStreamReader reader(xml);
    if (!enterRoot(reader, "status", &s.error))
        return s;
    s.etag = reader.attributes().value(QLatin1String("etag")).toString();

    while (reader.readNextStartElement()) {
        const QStringRef n = reader.name();
        if (n == QLatin1String("streamUrl")) {
            s.hasStreamUrl = true;
            reader.skipCurrentElement();
            continue;
        }
        if (n == QLatin1String("actions")) {
            while (reader.readNextStartElement()) {
                if (reader.name() == QLatin1String("action")) {
                    const QXmlStreamAttributes a = reader.attributes();
                    const QString name = a.value(QLatin1String("name")).toString();
                    const QString url = a.value(QLatin1String("url")).toString();
                    if (name == QLatin1String("skip"))
                        s.skipActionUrl = url;
                    else if (name == QLatin1String("back"))
                        s.backActionUrl = url;
                }
                reader.skipCurrentElement();
            }
            continue;
        }
        if (n == QLatin1String("state")) s.state = reader.readElementText();
        else if (n == QLatin1String("title1")) s.title1 = reader.readElementText();
        else if (n == QLatin1String("title2")) s.title2 = reader.readElementText();
        else if (n == QLatin1String("title3")) s.title3 = reader.readElementText();
        else if (n == QLatin1String("twoline_title1")) s.twolineTitle1 = reader.readElementText();
        else if (n == QLatin1String("twoline_title2")) s.twolineTitle2 = reader.readElementText();
        else if (n == QLatin1String("image")) s.image = reader.readElementText();
        else if (n == QLatin1String("syncStat")) s.syncStat = reader.readElementText();
        else if (n == QLatin1String("prid")) s.prid = reader.readElementText();
        else if (n == QLatin1String("pid")) s.pid = reader.readElementText();
        else if (n == QLatin1String("song")) s.song = toInt(reader.readElementText(), -1);
        else if (n == QLatin1String("secs")) s.secs = toInt(reader.readElementText(), -1);
        else if (n == QLatin1String("totlen")) s.totlen = toInt(reader.readElementText(), -1);
        else if (n == QLatin1String("groupVolume")) s.groupVolume = toInt(reader.readElementText(), -1);
        else if (n == QLatin1String("volume")) s.volume = toInt(reader.readElementText(), -1);
        else if (n == QLatin1String("mute")) s.mute = toBool(reader.readElementText());
        else reader.skipCurrentElement();
    }
    s.ok = finish(reader, &s.error);
    return s;
}

SyncStatus parseSyncStatus(const QByteArray &xml)
{
    SyncStatus s;
    QXmlStreamReader reader(xml);
    if (!enterRoot(reader, "SyncStatus", &s.error)) {
        if (reader.name() == QLatin1String("UpgradeStatusStage1"))
            s.upgradeStage = 1;
        else if (reader.name() == QLatin1String("UpgradeStatusStage2"))
            s.upgradeStage = 2;
        return s;
    }
    const QXmlStreamAttributes a = reader.attributes();
    s.etag = a.value(QLatin1String("etag")).toString();
    s.id = a.value(QLatin1String("id")).toString();
    s.name = a.value(QLatin1String("name")).toString();
    s.model = a.value(QLatin1String("model")).toString();
    s.modelName = a.value(QLatin1String("modelName")).toString();
    s.brand = a.value(QLatin1String("brand")).toString();
    s.mac = a.value(QLatin1String("mac")).toString();
    s.syncStat = a.value(QLatin1String("syncStat")).toString();
    s.group = a.value(QLatin1String("group")).toString();
    s.fixedGroup = a.value(QLatin1String("zone")).toString();
    s.fixedPrimary = toBool(a.value(QLatin1String("zoneMaster")).toString());
    s.fixedSecondary = toBool(a.value(QLatin1String("zoneSlave")).toString());
    s.volume = toInt(a.value(QLatin1String("volume")).toString(), -1);
    s.mute = toBool(a.value(QLatin1String("mute")).toString());
    s.version = a.value(QLatin1String("version")).toString();
    if (a.hasAttribute(QLatin1String("initialized")))
        s.initialized = toBool(a.value(QLatin1String("initialized")).toString());

    while (reader.readNextStartElement()) {
        const QStringRef n = reader.name();
        if (n == QLatin1String("master")) {
            const QXmlStreamAttributes ma = reader.attributes();
            s.primaryPort = toInt(ma.value(QLatin1String("port")).toString(), 11000);
            s.primaryReconnecting = toBool(ma.value(QLatin1String("reconnecting")).toString());
            s.primaryHost = reader.readElementText().trimmed();
        } else if (n == QLatin1String("slave")) {
            const QXmlStreamAttributes sa = reader.attributes();
            const QString host = sa.value(QLatin1String("id")).toString();
            const int port = toInt(sa.value(QLatin1String("port")).toString(), 11000);
            if (!host.isEmpty())
                s.secondaries.append(QStringLiteral("%1:%2").arg(host).arg(port));
            reader.skipCurrentElement();
        } else {
            reader.skipCurrentElement();
        }
    }
    s.ok = finish(reader, &s.error);
    return s;
}

QString Preset::inputId() const
{
    if (!url.startsWith(QLatin1String("Capture:")))
        return QString();
    const int q = url.indexOf(QLatin1Char('?'));
    if (q < 0)
        return QString();
    for (const QString &pair : url.mid(q + 1).split(QLatin1Char('&'))) {
        if (pair.startsWith(QLatin1String("id=")))
            return pair.mid(3);
    }
    return QString();
}

Playlist parsePlaylist(const QByteArray &xml)
{
    Playlist p;
    QXmlStreamReader reader(xml);
    if (!enterRoot(reader, "playlist", &p.error))
        return p;
    const QXmlStreamAttributes a = reader.attributes();
    p.length = toInt(a.value(QLatin1String("length")).toString(), -1);
    p.id = a.value(QLatin1String("id")).toString();
    while (reader.readNextStartElement()) {
        const QStringRef n = reader.name();
        if (n == QLatin1String("length")) {
            p.length = toInt(reader.readElementText(), -1);
        } else if (n == QLatin1String("id")) {
            p.id = reader.readElementText().trimmed();
        } else if (n == QLatin1String("song")) {
            QueueEntry e;
            const QXmlStreamAttributes sa = reader.attributes();
            e.position = toInt(sa.value(QLatin1String("id")).toString(), -1);
            e.songId = sa.value(QLatin1String("songid")).toString();
            while (reader.readNextStartElement()) {
                const QStringRef c = reader.name();
                if (c == QLatin1String("title")) e.title = reader.readElementText();
                else if (c == QLatin1String("art")) e.artist = reader.readElementText();
                else if (c == QLatin1String("alb")) e.album = reader.readElementText();
                else if (c == QLatin1String("fn")) e.fn = reader.readElementText();
                else if (c == QLatin1String("image")) e.image = reader.readElementText();
                else if (c == QLatin1String("time")) e.duration = toInt(reader.readElementText(), -1);
                else reader.skipCurrentElement();
            }
            if (e.position >= 0)
                p.songs.append(e);
        } else {
            reader.skipCurrentElement();
        }
    }
    p.ok = finish(reader, &p.error);
    return p;
}

namespace {

BrowseItem readBrowseItem(QXmlStreamReader &reader, const QString &category)
{
    BrowseItem item;
    const QXmlStreamAttributes a = reader.attributes();
    item.type = a.value(QLatin1String("type")).toString();
    item.text = a.value(QLatin1String("text")).toString();
    item.text2 = a.value(QLatin1String("text2")).toString();
    item.image = a.value(QLatin1String("image")).toString();
    item.browseKey = a.value(QLatin1String("browseKey")).toString();
    item.playUrl = a.value(QLatin1String("playURL")).toString();
    item.autoplayUrl = a.value(QLatin1String("autoplayURL")).toString();
    item.contextMenuKey = a.value(QLatin1String("contextMenuKey")).toString();
    item.inputType = a.value(QLatin1String("inputType")).toString();
    item.duration = toInt(a.value(QLatin1String("duration")).toString(), -1);
    item.category = category;
    while (reader.readNextStartElement()) {
        if (reader.name() == QLatin1String("contextMenu")) {
            while (reader.readNextStartElement()) {
                if (reader.name() == QLatin1String("item")) {
                    const QXmlStreamAttributes m = reader.attributes();
                    BrowseAction action;
                    action.type = m.value(QLatin1String("type")).toString();
                    action.text = m.value(QLatin1String("text")).toString();
                    action.actionUrl = m.value(QLatin1String("actionURL")).toString();
                    if (!action.type.isEmpty() && !action.actionUrl.isEmpty())
                        item.actions.append(action);
                }
                reader.skipCurrentElement();
            }
        } else {
            reader.skipCurrentElement();
        }
    }
    return item;
}

} // namespace

Browse parseBrowse(const QByteArray &xml)
{
    Browse b;
    QXmlStreamReader reader(xml);
    if (!reader.readNextStartElement()) {
        b.error = reader.hasError() ? reader.errorString() : QStringLiteral("empty document");
        return b;
    }
    if (reader.name() == QLatin1String("error")) {
        QString message;
        while (reader.readNextStartElement()) {
            if (reader.name() == QLatin1String("message"))
                message = reader.readElementText();
            else
                reader.skipCurrentElement();
        }
        b.playerError = true;
        b.error = message.isEmpty() ? QStringLiteral("error") : message;
        return b;
    }
    if (reader.name() != QLatin1String("browse")) {
        b.error = QStringLiteral("unexpected root element <%1>").arg(reader.name().toString());
        return b;
    }
    const QXmlStreamAttributes a = reader.attributes();
    b.type = a.value(QLatin1String("type")).toString();
    b.serviceName = a.value(QLatin1String("serviceName")).toString();
    b.serviceIcon = a.value(QLatin1String("serviceIcon")).toString();
    b.searchKey = a.value(QLatin1String("searchKey")).toString();
    b.nextKey = a.value(QLatin1String("nextKey")).toString();
    b.parentKey = a.value(QLatin1String("parentKey")).toString();
    b.playUrl = a.value(QLatin1String("playURL")).toString();
    while (reader.readNextStartElement()) {
        if (reader.name() == QLatin1String("item")) {
            b.items.append(readBrowseItem(reader, QString()));
        } else if (reader.name() == QLatin1String("category")) {
            const QString title = reader.attributes().value(QLatin1String("text")).toString();
            while (reader.readNextStartElement()) {
                if (reader.name() == QLatin1String("item"))
                    b.items.append(readBrowseItem(reader, title));
                else
                    reader.skipCurrentElement();
            }
        } else {
            reader.skipCurrentElement();
        }
    }
    b.ok = finish(reader, &b.error);
    return b;
}

Presets parsePresets(const QByteArray &xml)
{
    Presets p;
    QXmlStreamReader reader(xml);
    if (!enterRoot(reader, "presets", &p.error))
        return p;
    p.prid = reader.attributes().value(QLatin1String("prid")).toString();
    while (reader.readNextStartElement()) {
        if (reader.name() == QLatin1String("preset")) {
            const QXmlStreamAttributes a = reader.attributes();
            bool ok = false;
            Preset preset;
            preset.id = a.value(QLatin1String("id")).toString().toInt(&ok);
            preset.name = a.value(QLatin1String("name")).toString();
            preset.image = a.value(QLatin1String("image")).toString();
            preset.url = a.value(QLatin1String("url")).toString();
            if (ok)
                p.items.append(preset);
        }
        reader.skipCurrentElement();
    }
    p.ok = finish(reader, &p.error);
    return p;
}

Inputs parseCaptureInputs(const QByteArray &xml)
{
    Inputs inputs;
    QXmlStreamReader reader(xml);
    if (!enterRoot(reader, "radiotime", &inputs.error))
        return inputs;
    while (reader.readNextStartElement()) {
        // Hub inputs come as <remoteitem> inside <category> and are skipped
        // with it; so is any item without an inputType (cloud services).
        if (reader.name() == QLatin1String("item")) {
            const QXmlStreamAttributes a = reader.attributes();
            Input input;
            input.inputType = a.value(QLatin1String("inputType")).toString();
            input.id = a.value(QLatin1String("id")).toString();
            input.text = a.value(QLatin1String("text")).toString();
            input.image = a.value(QLatin1String("image")).toString();
            input.url = a.value(QLatin1String("URL")).toString();
            if (!input.inputType.isEmpty() && !input.url.isEmpty())
                inputs.items.append(input);
        }
        reader.skipCurrentElement();
    }
    inputs.ok = finish(reader, &inputs.error);
    return inputs;
}


GroupAdd parseGroupAdd(const QByteArray &xml)
{
    GroupAdd s;
    QXmlStreamReader reader(xml);
    if (!enterRoot(reader, "addSlave", &s.error))
        return s;
    while (reader.readNextStartElement()) {
        if (reader.name() == QLatin1String("slave")) {
            const QXmlStreamAttributes a = reader.attributes();
            const QString host = a.value(QLatin1String("id")).toString();
            const int port = toInt(a.value(QLatin1String("port")).toString(), 11000);
            if (!host.isEmpty())
                s.added.append(QStringLiteral("%1:%2").arg(host).arg(port));
        }
        reader.skipCurrentElement();
    }
    s.ok = finish(reader, &s.error);
    return s;
}

VolumeReply parseVolume(const QByteArray &xml)
{
    VolumeReply v;
    QXmlStreamReader reader(xml);
    if (!enterRoot(reader, "volume", &v.error))
        return v;
    v.mute = toBool(reader.attributes().value(QLatin1String("mute")).toString());
    v.level = toInt(reader.readElementText(), -1);
    v.ok = finish(reader, &v.error) && v.level >= 0;
    return v;
}

UpgradeCheck parseUpgradeCheck(const QByteArray &xml)
{
    UpgradeCheck u;
    QXmlStreamReader reader(xml);
    if (!enterRoot(reader, "upgrade", &u.error))
        return u;
    const QXmlStreamAttributes a = reader.attributes();
    u.available = toBool(a.value(QLatin1String("available")).toString());
    u.inProgress = toBool(a.value(QLatin1String("inProgress")).toString());
    u.version = a.value(QLatin1String("version")).toString();
    u.ok = finish(reader, &u.error);
    return u;
}

} // namespace BluOS
