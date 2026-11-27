/*  Talonflame Switcher — reads/writes the config block inside wafel_inkay.ipx
    Copyright 2026 Talonflame contributors
    SPDX-License-Identifier: GPL-3.0-or-later
*/
#pragma once

#include <cstdint>
#include <string>

#define IPX_PATH "fs:/vol/external01/wiiu/ios_plugins/wafel_inkay.ipx"

struct IpxConfig {
    bool found = false;
    std::string error;
    long offset = 0; // file offset of the magic
    uint32_t profile = 0;
    uint32_t ssl_mode = 0;
    uint32_t flags = 0;
};

IpxConfig ipxLoad(const char *path = IPX_PATH);
// Rewrites profile/ssl_mode in place. Returns false and sets cfg.error on failure.
bool ipxSave(IpxConfig &cfg, uint32_t profile, uint32_t ssl_mode, const char *path = IPX_PATH);
