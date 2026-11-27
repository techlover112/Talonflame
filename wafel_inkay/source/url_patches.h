/*  Talonflame — wafel_inkay: boot-time network URL patches for ISFShax/stroopwafel
    URL tables derived from Inkay (Pretendo Network contributors) and
    Bnkay (Brewtendo Network contributors).

    Copyright 2022-2024 Pretendo Network contributors <pretendo.network>
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
*/

#ifndef URL_PATCHES_H
#define URL_PATCHES_H

#include <stdint.h>

typedef struct {
    uint32_t address; /* IOSU virtual address (5.5.x) */
    const char *url;
} url_patch;

/* IOS-NIM-BOSS .rodata URL slots (IOSU virtual addresses, FW 5.5.x).
 * The two .bss entries Inkay/Bnkay patch at runtime (0xE24B8A24 nppl policy
 * list URL, 0xE31930D4 account API format string) are intentionally absent:
 * at boot time .bss has not been populated yet. The policy-list URL is
 * rebuilt from the (patched) nppl hostname in .rodata; the account API
 * format string is handled by a pattern search, see ACCOUNT_URL_* below.
 */

/* Brewtendo Network (from BrewtendoNetwork/Bnkay, TVii entries ignored) */
static const url_patch url_patches_brewtendo[] = {
    {0xE2282550, "http://pushmore.wup.shop.brewtendo.org/pushmore/r/%s"},
    {0xE229A0A0, "http://npns-dev.c.app.brewtendo.org/bst.dat"},
    {0xE229A0D0, "http://npns-dev.c.app.brewtendo.org/bst2.dat"},
    {0xE2281964, "https://tagaya.wup.shop.brewtendo.org/tagaya/versionlist/%s/%s/%s"},
    {0xE22819B4, "https://tagaya.wup.shop.brewtendo.org/tagaya/versionlist/%s/%s/latest_version"},
    {0xE2282584, "http://pushmo.wup.shop.brewtendo.org/pushmo/d/%s/%u"},
    {0xE22825B8, "http://pushmo.wup.shop.brewtendo.org/pushmo/c/%u/%u"},
    {0xE2282DB4, "https://ecs.wup.sh.brewtendo.org/ecs/services/ECommerceSOAP"},
    {0xE22830A0, "https://ecs.wup.sh.brewtendo.org/ecs/services/ECommerceSOAP"},
    {0xE22830E0, "https://nus.wup.sh.brewtendo.org/nus/services/NetUpdateSOAP"},
    {0xE2299990, "nppl.app.brewtendo.org"},
    {0xE229A600, "https://pls.wup.shop.brewtendo.org/pls/upload"},
    {0xE229A6AC, "https://npvk-dev.app.brewtendo.org/reports"},
    {0xE229A6D8, "https://npvk.app.brewtendo.org/reports"},
    {0xE229B1F4, "https://npts.app.brewtendo.org/p01/tasksheet/%s/%s/%s/%s?c=%s&l=%s"},
    {0xE229B238, "https://npts.app.brewtendo.org/p01/tasksheet/%s/%s/%s?c=%s&l=%s"},
    {0xE22AB2D8, "https://idbe-wup.cdn.nintendo.net/icondata/%02X/%016llX.idbe"},
    {0xE22AB318, "https://idbe-ctr.cdn.brewtendo.org/icondata/%02X/%016llX.idbe"},
    {0xE22AB358, "https://idbe-wup.cdn.nintendo.net/icondata/%02X/%016llX-%d.idbe"},
    {0xE22AB398, "https://idbe-ctr.cdn.brewtendo.org/icondata/%02X/%016llX-%d.idbe"},
    {0xE22B3EF8, "https://ecs.c.shop.brewtendo.org"},
    {0xE22B3F30, "https://ecs.c.shop.brewtendo.org/ecs/services/ECommerceSOAP"},
    {0xE22B3F70, "https://ias.c.shop.brewtendo.org/ias/services/IdentityAuthenticationSOAP"},
    {0xE22B3FBC, "https://cas.c.shop.brewtendo.org/cas/services/CatalogingSOAP"},
    {0xE22B3FFC, "https://nus.c.shop.brewtendo.org/nus/services/NetUpdateSOAP"},
    {0xE229DE0C, "n.app.brewtendo.org"},
};

/* Pretendo Network (from PretendoNetwork/Inkay) */
static const url_patch url_patches_pretendo[] = {
    {0xE2282550, "http://pushmore.wup.shop.theelijohnson.cc/pushmore/r/%s"},
    {0xE229A0A0, "http://npns-dev.c.boss.theelijohnson.cc/bst.dat"},
    {0xE229A0D0, "http://npns-dev.c.boss.theelijohnson.cc/bst2.dat"},
    {0xE2281964, "https://tagaya.wup.shop.theelijohnson.cc/tagaya/versionlist/%s/%s/%s"},
    {0xE22819B4, "https://tagaya.wup.shop.theelijohnson.cc/tagaya/versionlist/%s/%s/latest_version"},
    {0xE2282584, "http://pushmo.wup.shop.theelijohnson.cc/pushmo/d/%s/%u"},
    {0xE22825B8, "http://pushmo.wup.shop.theelijohnson.cc/pushmo/c/%u/%u"},
    {0xE2282DB4, "https://ecs.wup.shop.theelijohnson.cc/ecs/services/ECommerceSOAP"},
    {0xE22830A0, "https://ecs.wup.shop.theelijohnson.cc/ecs/services/ECommerceSOAP"},
    {0xE22830E0, "https://nus.wup.shop.theelijohnson.cc/nus/services/NetUpdateSOAP"},
    {0xE2299990, "nppl.boss.theelijohnson.cc"},
    {0xE229A600, "https://pls.wup.shop.theelijohnson.cc/pls/upload"},
    {0xE229A6AC, "https://npvk-dev.boss.theelijohnson.cc/reports"},
    {0xE229A6D8, "https://npvk.boss.theelijohnson.cc/reports"},
    {0xE229B1F4, "https://npts.boss.theelijohnson.cc/p01/tasksheet/%s/%s/%s/%s?c=%s&l=%s"},
    {0xE229B238, "https://npts.boss.theelijohnson.cc/p01/tasksheet/%s/%s/%s?c=%s&l=%s"},
    {0xE22AB2D8, "https://idbe-wup.cdn.theelijohnson.cc/icondata/%02X/%016llX.idbe"},
    {0xE22AB318, "https://idbe-ctr.cdn.theelijohnson.cc/icondata/%02X/%016llX.idbe"},
    {0xE22AB358, "https://idbe-wup.cdn.theelijohnson.cc/icondata/%02X/%016llX-%d.idbe"},
    {0xE22AB398, "https://idbe-ctr.cdn.theelijohnson.cc/icondata/%02X/%016llX-%d.idbe"},
    {0xE22B3EF8, "https://ecs.c.shop.theelijohnson.cc"},
    {0xE22B3F30, "https://ecs.c.shop.theelijohnson.cc/ecs/services/ECommerceSOAP"},
    {0xE22B3F70, "https://ias.c.shop.theelijohnson.cc/ias/services/IdentityAuthenticationSOAP"},
    {0xE22B3FBC, "https://cas.c.shop.theelijohnson.cc/cas/services/CatalogingSOAP"},
    {0xE22B3FFC, "https://nus.c.shop.theelijohnson.cc/nus/services/NetUpdateSOAP"},
    {0xE229DE0C, "n.app.theelijohnson.cc"},
};

/* The account-API format string lives in .rodata somewhere (its runtime copy
 * is the .bss slot 0xE31930D4 that Inkay patches post-boot). At boot we patch
 * the .rodata source by pattern search so the .bss copy is built correctly. */
#define ACCOUNT_URL_ORIG      "https://%s%saccount.nintendo.net/v%u/api/"
#define ACCOUNT_URL_BREWTENDO "https://%s%saccount.brewtendo.org/v%u/api/"
#define ACCOUNT_URL_PRETENDO  "https://%s%saccount.pretendo.cc/v%u/api/"

/* IOS-NET NSSL certificate-verification bypass ("NoSSL"), from Inkay:
 * one `mov r0, #1`. The location moved in 5.5.5. */
#define NSSL_VERIFY_PATCH_ADDR_555    0xE1019F78u /* FW >= 5.5.5 */
#define NSSL_VERIFY_PATCH_ADDR_LEGACY 0xE1019E84u /* FW 5.5.0 - 5.5.4 */
#define NSSL_VERIFY_PATCH_VALUE       0xE3A00001u /* mov r0, #1 */

#endif /* URL_PATCHES_H */
