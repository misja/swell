// SPDX-License-Identifier: MIT
// The browse model as the QML pages see it: letters, the other links of an
// expanded menu and each row's actions as lists of maps, which JavaScript
// reads as arrays of objects (spec 007). Registered as BrowseModel.
#ifndef SWELL_QMLBROWSEMODEL_H
#define SWELL_QMLBROWSEMODEL_H

#include <QVariantList>

#include "browsemodel.h"

namespace Swell {

class QmlBrowseModel : public BluOS::BrowseModel
{
    Q_OBJECT
    // For a letter index: [{ "text": "A", "row": 17 }, ...].
    Q_PROPERTY(QVariantList letters READ qmlLetters NOTIFY qmlLettersChanged)
    // [{ "text", "browseKey" }, ...]
    Q_PROPERTY(QVariantList moreLinks READ qmlMoreLinks NOTIFY qmlMoreLinksChanged)

public:
    explicit QmlBrowseModel(QObject *parent = nullptr);

    // ActionsRole as [{ "type", "text", "url" }, ...]; other roles as given.
    QVariant data(const QModelIndex &index, int role) const override;

    QVariantList qmlLetters() const;
    QVariantList qmlMoreLinks() const;

signals:
    // moc in Qt 5.6 needs a NOTIFY signal of this class (evidence.md).
    void qmlLettersChanged();
    void qmlMoreLinksChanged();
};

} // namespace Swell

#endif
