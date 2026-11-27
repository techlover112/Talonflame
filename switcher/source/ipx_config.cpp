/*  Talonflame Switcher — reads/writes the config block inside wafel_inkay.ipx
    Copyright 2026 Talonflame contributors
    SPDX-License-Identifier: GPL-3.0-or-later
*/
#include "ipx_config.h"
#include "fsutil.h"
#include "tlnf_config.h"

#include <cstdio>
#include <cstring>
#include <vector>

// The .ipx is a big-endian ELF and the Wii U PPC is big-endian, so fields
// can be read and written as native uint32_t.
static uint32_t rd32(const uint8_t *p) {
    uint32_t v;
    memcpy(&v, p, 4);
    return v;
}

IpxConfig ipxLoad(const char *path) {
    IpxConfig cfg;
    std::vector<uint8_t> data;
    if (!fsutil::readFile(path, data)) {
        cfg.error = "wafel_inkay.ipx not found in sd:/wiiu/ios_plugins/";
        return cfg;
    }

    int matches = 0;
    for (size_t i = 0; i + sizeof(tlnf_config_t) <= data.size(); i++) {
        if (memcmp(&data[i], TLNF_MAGIC, TLNF_MAGIC_LEN) != 0) {
            continue;
        }
        const uint8_t *p = &data[i];
        if (rd32(p + 0x08) != 1) {
            continue;
        }
        matches++;
        cfg.offset = (long)i;
        cfg.profile = rd32(p + 0x0C);
        cfg.ssl_mode = rd32(p + 0x10);
        cfg.flags = rd32(p + 0x14);
    }

    if (matches != 1) {
        cfg.error = matches == 0 ? "config block not found in wafel_inkay.ipx"
                                 : "multiple config blocks in wafel_inkay.ipx";
        return cfg;
    }
    if (cfg.profile > TLNF_PROFILE_PRETENDO || cfg.ssl_mode > TLNF_SSL_LEGACY) {
        cfg.error = "config block in wafel_inkay.ipx has invalid values";
        return cfg;
    }
    cfg.found = true;
    return cfg;
}

bool ipxSave(IpxConfig &cfg, uint32_t profile, uint32_t ssl_mode, const char *path) {
    if (!cfg.found) {
        return false;
    }
    FILE *f = fopen(path, "r+b");
    if (!f) {
        cfg.error = "could not open wafel_inkay.ipx for writing";
        return false;
    }
    uint32_t fields[2] = {profile, ssl_mode};
    bool ok = fseek(f, cfg.offset + 0x0C, SEEK_SET) == 0 &&
              fwrite(fields, 4, 2, f) == 2;
    if (fclose(f) != 0) {
        ok = false;
    }
    if (!ok) {
        cfg.error = "write to wafel_inkay.ipx failed";
        return false;
    }

    IpxConfig check = ipxLoad(path);
    if (!check.found || check.profile != profile || check.ssl_mode != ssl_mode) {
        cfg.error = "wafel_inkay.ipx verification failed after writing";
        return false;
    }
    cfg = check;
    return true;
}
