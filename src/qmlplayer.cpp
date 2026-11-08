// SPDX-License-Identifier: MIT
#include "qmlplayer.h"

#include <QVariantMap>

namespace Swell {

QmlPlayer::QmlPlayer(QNetworkAccessManager *nam, const QString &host, int port, QObject *parent)
    : BluOS::Client(nam, host, port, parent)
{
    connect(this, &BluOS::Client::presetsChanged, this, &QmlPlayer::qmlPresetsChanged);
    connect(this, &BluOS::Client::inputsChanged, this, &QmlPlayer::qmlInputsChanged);
}

QVariantList QmlPlayer::qmlPresets() const
{
    QVariantList list;
    for (const BluOS::Preset &p : presets()) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), p.id);
        item.insert(QStringLiteral("name"), p.name);
        item.insert(QStringLiteral("image"), resolveImage(p.image));
        // A preset that selects an input; Home shows that input's own icon.
        item.insert(QStringLiteral("inputId"), p.inputId());
        list.append(item);
    }
    return list;
}

QVariantList QmlPlayer::qmlInputs() const
{
    QVariantList list;
    for (const BluOS::Input &in : inputs()) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), in.id);
        item.insert(QStringLiteral("text"), in.text);
        item.insert(QStringLiteral("inputType"), in.inputType);
        item.insert(QStringLiteral("image"), resolveImage(in.image));
        item.insert(QStringLiteral("url"), in.url);
        list.append(item);
    }
    return list;
}

BluOS::Client *QmlPlayerModel::newClient(QNetworkAccessManager *nam, const QString &host, int port,
                                         QObject *parent)
{
    return new QmlPlayer(nam, host, port, parent);
}

} // namespace Swell
