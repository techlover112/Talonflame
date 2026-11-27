/*  Talonflame Switcher — reads back what wafel_inkay patched at this boot
    Copyright 2026 Talonflame contributors
    SPDX-License-Identifier: GPL-3.0-or-later
*/
#include "bootstatus.h"
#include "tlnf_config.h"
#include "url_patches.h"

#include <mocha/mocha.h>

#include <cstring>

namespace {

constexpr uint32_t NAPP_HOST_ADDR = 0xE229DE0C;

bool readIosuString(uint32_t addr, char *out, size_t size) {
    for (size_t i = 0; i < size / 4; i++) {
        uint32_t w = 0;
        if (Mocha_IOSUKernelRead32(addr + i * 4, &w) != MOCHA_RESULT_SUCCESS) {
            return false;
        }
        memcpy(out + i * 4, &w, 4);
    }
    out[size - 1] = '\0';
    return true;
}

} // namespace

BootStatus readBootStatus(uint32_t ssl_mode) {
    BootStatus st;

    char host[32] = {};
    if (!readIosuString(NAPP_HOST_ADDR, host, sizeof(host))) {
        return st;
    }
    st.host = host;
    if (st.host == "n.app.brewtendo.org") {
        st.network = TLNF_PROFILE_BREWTENDO;
    } else if (st.host == "n.app.pretendo.cc") {
        st.network = TLNF_PROFILE_PRETENDO;
    } else {
        st.network = TLNF_PROFILE_NINTENDO;
    }

    st.sslAddr = (ssl_mode == TLNF_SSL_LEGACY) ? NSSL_VERIFY_PATCH_ADDR_LEGACY
                                               : NSSL_VERIFY_PATCH_ADDR_555;
    if (Mocha_IOSUKernelRead32(st.sslAddr, &st.sslWord) != MOCHA_RESULT_SUCCESS) {
        return st;
    }
    st.sslPatched = st.sslWord == NSSL_VERIFY_PATCH_VALUE;
    st.readable = true;
    return st;
}
