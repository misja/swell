// SPDX-License-Identifier: MIT
#include "registration.h"

#include <QtQml>

#include "browsemodel.h"
#include "client.h"
#include "playermodel.h"
#include "queuemodel.h"

namespace Swell {

void registerTypes()
{
    qmlRegisterUncreatableType<BluOS::Client>("Swell", 1, 0, "Player",
                                              QStringLiteral("Players come from the player model"));
    qmlRegisterUncreatableType<BluOS::PlayerModel>("Swell", 1, 0, "PlayerModel",
                                                   QStringLiteral("The app provides the player model"));
    qmlRegisterUncreatableType<BluOS::QueueModel>("Swell", 1, 0, "QueueModel",
                                                  QStringLiteral("Each player provides its queue"));
    // One per Browse page, owned by that page.
    qmlRegisterType<BluOS::BrowseModel>("Swell", 1, 0, "BrowseModel");
}

} // namespace Swell
