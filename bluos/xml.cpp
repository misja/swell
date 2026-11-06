// SPDX-License-Identifier: MIT
#include "xml.h"

#include <QXmlStreamReader>

namespace bluos {

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
        if (n == QLatin1String("state")) s.state = reader.readElementText();
        else if (n == QLatin1String("title1")) s.title1 = reader.readElementText();
        else if (n == QLatin1String("title2")) s.title2 = reader.readElementText();
        else if (n == QLatin1String("title3")) s.title3 = reader.readElementText();
        else if (n == QLatin1String("twoline_title1")) s.twolineTitle1 = reader.readElementText();
        else if (n == QLatin1String("twoline_title2")) s.twolineTitle2 = reader.readElementText();
        else if (n == QLatin1String("image")) s.image = reader.readElementText();
        else if (n == QLatin1String("syncStat")) s.syncStat = reader.readElementText();
        else if (n == QLatin1String("prid")) s.prid = reader.readElementText();
        else if (n == QLatin1String("secs")) s.secs = toInt(reader.readElementText(), -1);
        else if (n == QLatin1String("totlen")) s.totlen = toInt(reader.readElementText(), -1);
        else if (n == QLatin1String("volume")) s.volume = toInt(reader.readElementText(), -1);
        else if (n == QLatin1String("mute")) s.mute = toBool(reader.readElementText());
        else if (n == QLatin1String("canSeek")) s.canSeek = toBool(reader.readElementText());
        else reader.skipCurrentElement();
    }
    s.ok = finish(reader, &s.error);
    return s;
}

SyncStatus parseSyncStatus(const QByteArray &xml)
{
    SyncStatus s;
    QXmlStreamReader reader(xml);
    if (!enterRoot(reader, "SyncStatus", &s.error))
        return s;
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
    s.volume = toInt(a.value(QLatin1String("volume")).toString(), -1);
    s.mute = toBool(a.value(QLatin1String("mute")).toString());
    if (a.hasAttribute(QLatin1String("initialized")))
        s.initialized = toBool(a.value(QLatin1String("initialized")).toString());

    while (reader.readNextStartElement()) {
        const QStringRef n = reader.name();
        if (n == QLatin1String("master")) {
            const QXmlStreamAttributes ma = reader.attributes();
            s.masterPort = toInt(ma.value(QLatin1String("port")).toString(), 11000);
            s.masterReconnecting = toBool(ma.value(QLatin1String("reconnecting")).toString());
            s.masterHost = reader.readElementText().trimmed();
        } else if (n == QLatin1String("slave")) {
            const QXmlStreamAttributes sa = reader.attributes();
            const QString host = sa.value(QLatin1String("id")).toString();
            const int port = toInt(sa.value(QLatin1String("port")).toString(), 11000);
            if (!host.isEmpty())
                s.slaves.append(QStringLiteral("%1:%2").arg(host).arg(port));
            reader.skipCurrentElement();
        } else {
            reader.skipCurrentElement();
        }
    }
    s.ok = finish(reader, &s.error);
    return s;
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

Volume parseVolume(const QByteArray &xml)
{
    Volume v;
    QXmlStreamReader reader(xml);
    if (!enterRoot(reader, "volume", &v.error))
        return v;
    const QXmlStreamAttributes a = reader.attributes();
    v.etag = a.value(QLatin1String("etag")).toString();
    v.mute = toBool(a.value(QLatin1String("mute")).toString());
    v.db = a.value(QLatin1String("db")).toString().toDouble();
    v.level = toInt(reader.readElementText(QXmlStreamReader::SkipChildElements), -1);
    v.ok = finish(reader, &v.error);
    return v;
}

} // namespace bluos
