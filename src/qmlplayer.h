// SPDX-License-Identifier: MIT
// The player and player model as the QML pages see them: presets and inputs
// as lists of maps, which JavaScript reads as arrays of objects. Qt 5.6 does
// not hand typed lists to JavaScript (spec 007).
#ifndef SWELL_QMLPLAYER_H
#define SWELL_QMLPLAYER_H

#include <QVariantList>

#include "client.h"
#include "playermodel.h"

namespace Swell {

class QmlPlayer : public BluOS::Client
{
    Q_OBJECT
    // Each: id (int), name, image (resolved URL), inputId (the capture input
    // it selects, or empty).
    Q_PROPERTY(QVariantList presets READ qmlPresets NOTIFY qmlPresetsChanged)
    // Each: id, text, inputType, image (resolved URL), url (encoded, as given).
    Q_PROPERTY(QVariantList inputs READ qmlInputs NOTIFY qmlInputsChanged)

public:
    QmlPlayer(QNetworkAccessManager *nam, const QString &host, int port, QObject *parent);

    QVariantList qmlPresets() const;
    QVariantList qmlInputs() const;

signals:
    // moc in Qt 5.6 needs a NOTIFY signal of this class (evidence.md).
    void qmlPresetsChanged();
    void qmlInputsChanged();
};

class QmlPlayerModel : public BluOS::PlayerModel
{
    Q_OBJECT

public:
    using BluOS::PlayerModel::PlayerModel;

protected:
    BluOS::Client *newClient(QNetworkAccessManager *nam, const QString &host, int port,
                             QObject *parent) override;
};

} // namespace Swell

#endif
