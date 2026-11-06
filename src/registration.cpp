// SPDX-License-Identifier: MIT
#include "registration.h"

#include <QtQml>

#include "client.h"
#include "playermodel.h"

namespace swell {

void registerTypes()
{
    qmlRegisterUncreatableType<bluos::Client>("Swell", 1, 0, "Player",
                                              QStringLiteral("Players come from the player model"));
    qmlRegisterUncreatableType<bluos::PlayerModel>("Swell", 1, 0, "PlayerModel",
                                                   QStringLiteral("The app provides the player model"));
}

} // namespace swell
