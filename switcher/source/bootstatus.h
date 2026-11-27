/*  Talonflame Switcher — reads back what wafel_inkay patched at this boot
    Copyright 2026 Talonflame contributors
    SPDX-License-Identifier: GPL-3.0-or-later
*/
#pragma once

#include <cstdint>
#include <string>

struct BootStatus {
    bool readable = false;   // false if Mocha could not read IOSU memory
    int network = -1;        // enum tlnf_profile, -1 = unrecognized hostname
    std::string host;        // the n.app hostname currently in IOSU
    bool sslPatched = false;
    uint32_t sslAddr = 0;
    uint32_t sslWord = 0;    // instruction currently at the SSL patch site
};

// Requires Mocha to be initialized (accountsInit does this).
BootStatus readBootStatus(uint32_t ssl_mode);
