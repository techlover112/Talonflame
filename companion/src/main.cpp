/*  Talonflame Companion — Aroma module (.wms)
    The PPC-side half of Inkay/Bnkay for consoles running wafel_inkay under
    ISFShax. wafel_inkay patches IOSU at boot; this module applies the
    patches that live in the Wii U's own software (DNS, eShop, Miiverse,
    account settings, game matchmaking/peer-to-peer).

    The patch sources in src/patches are from Bnkay (Brewtendo Network
    contributors), itself a fork of Inkay (Pretendo Network contributors).
    TVii is intentionally not included; a separate app handles it.

    Copyright 2022 Pretendo Network contributors <pretendo.network>
    Copyright 2026 Brewtendo Network contributors
    Copyright 2026 Talonflame contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"
#include "Notification.h"
#include "lang.h"
#include "sysconfig.h"
#include "patches/account_settings.h"
#include "patches/dns_hooks.h"
#include "patches/eshop_applet.h"
#include "patches/game_matchmaking.h"
#include "patches/game_peertopeer.h"
#include "patches/olv_applet.h"
#include "patches/olv_urls.h"
#include "utils/logger.h"

#include <wums.h>

#include <function_patcher/function_patching.h>
#include <mocha/mocha.h>
#include <notifications/notifications.h>

#include <cstdint>
#include <cstring>

#define COMPANION_VERSION "v1.0.0"

WUMS_MODULE_EXPORT_NAME("talonflame_companion");
WUMS_MODULE_DESCRIPTION("Talonflame PPC-side network patches (pairs with wafel_inkay)");
WUMS_MODULE_VERSION(COMPANION_VERSION);
WUMS_MODULE_AUTHOR("Talonflame, Brewtendo & Pretendo contributors");
WUMS_MODULE_LICENSE("GPLv3");

WUMS_DEPENDS_ON(homebrew_functionpatcher);
WUMS_DEPENDS_ON(homebrew_kernel);
WUMS_DEPENDS_ON(homebrew_notifications);

WUMS_USE_WUT_DEVOPTAB();

namespace {

enum class Network { Nintendo, Brewtendo, Pretendo };

Network gNetwork = Network::Nintendo;
bool gPatchesInstalled = false;
bool gNotified = false;

// wafel_inkay rewrites the IOS-NIM-BOSS "n.app" hostname at this IOSU
// address at boot (see wafel_inkay/source/url_patches.h). Reading it back
// tells us which network this boot is actually patched for, so the two
// halves can never disagree, even if the switcher changed the setting
// after boot (that change only applies on the next boot).
constexpr uint32_t NAPP_HOST_ADDR = 0xE229DE0C;

bool readIosuString(uint32_t addr, char *out, size_t size) {
    size_t words = size / 4;
    for (size_t i = 0; i < words; i++) {
        uint32_t w = 0;
        if (Mocha_IOSUKernelRead32(addr + i * 4, &w) != MOCHA_RESULT_SUCCESS) {
            return false;
        }
        memcpy(out + i * 4, &w, 4);
    }
    out[size - 1] = '\0';
    return true;
}

Network detectNetwork() {
    char host[32] = {};
    if (!readIosuString(NAPP_HOST_ADDR, host, sizeof(host))) {
        DEBUG_FUNCTION_LINE("could not read IOSU memory; assuming Nintendo Network");
        return Network::Nintendo;
    }
    if (strcmp(host, "n.app.brewtendo.org") == 0) {
        return Network::Brewtendo;
    }
    if (strcmp(host, "n.app.pretendo.cc") == 0) {
        return Network::Pretendo;
    }
    return Network::Nintendo;
}

const char *networkMessage(Network net) {
    auto strings = get_config_strings(get_system_language());
    switch (net) {
    case Network::Brewtendo: return strings.using_brewtendo_network.data();
    case Network::Pretendo:  return strings.using_pretendo_network.data();
    default:                 return strings.using_nintendo_network.data();
    }
}

} // namespace

WUMS_INITIALIZE() {
    WHBLogCafeInit();
    WHBLogUdpInit();

    if (const auto res = Mocha_InitLibrary(); res != MOCHA_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("Mocha init failed with code %d", res);
        return;
    }
    if (NotificationModule_InitLibrary() != NOTIFICATION_MODULE_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("NotificationModule_InitLibrary failed");
    }

    gNetwork = detectNetwork();

    // Bnkay's patch code reads these two flags at runtime.
    Config::connect_to_network = gNetwork != Network::Nintendo;
    Config::connect_to_brewtendo = gNetwork == Network::Brewtendo;
    Config::enable_tvii = false;
    Config::initialized = true;
    Config::plugin_is_loaded = true;

    if (!Config::connect_to_network) {
        return;
    }

    if (FunctionPatcher_InitLibrary() != FUNCTION_PATCHER_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE("FunctionPatcher_InitLibrary failed");
        return;
    }
    patchDNS();
    patchEshop();
    patchOlvApplet();
    patchAccountSettings();
    install_matchmaking_patches();
    gPatchesInstalled = true;
}

WUMS_DEINITIALIZE() {
    if (gPatchesInstalled) {
        unpatchDNS();
        unpatchEshop();
        unpatchOlvApplet();
        unpatchAccountSettings();
        remove_matchmaking_patches();
        FunctionPatcher_DeInitLibrary();
        gPatchesInstalled = false;
    }
    NotificationModule_DeInitLibrary();
    Mocha_DeInitLibrary();

    WHBLogCafeDeinit();
    WHBLogUdpDeinit();
}

WUMS_ALL_APPLICATION_STARTS_DONE() {
    // Per-title patches; each checks Config::connect_to_network itself.
    setup_olv_libs();
    peertopeer_patch();
    matchmaking_notify_titleswitch();
    hotpatchAccountSettings();

    if (!gNotified) {
        ShowNotification(networkMessage(gNetwork));
        gNotified = true;
    }
}
