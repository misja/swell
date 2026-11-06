// SPDX-License-Identifier: MIT
#include "registration.h"

#include <QtQml>

#include "client.h"
#include "playermodel.h"

namespace Swell {

void registerTypes()
{
    qmlRegisterUncreatableType<BluOS::Client>("Swell", 1, 0, "Player",
                                              QStringLiteral("Players come from the player model"));
    qmlRegisterUncreatableType<BluOS::PlayerModel>("Swell", 1, 0, "PlayerModel",
                                                   QStringLiteral("The app provides the player model"));
}

} // namespace Swell
