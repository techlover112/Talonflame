/*  Talonflame — wafel_inkay embedded configuration block.
    Copyright 2026 Talonflame contributors
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "tlnf_config.h"

/* Kept in .data (not .rodata) so the switcher can also flip it in memory in
 * the future, and 'used' so the linker never drops it. The initializer is
 * what ships in the .ipx file; the switcher app rewrites these bytes. */
__attribute__((section(".data"), used, aligned(16)))
volatile tlnf_config_t tlnf_config = {
    .magic          = {'T', 'L', 'N', 'F', 'C', 'F', 'G', '1'},
    .struct_version = 1,
    .profile        = TLNF_PROFILE_BREWTENDO,
    .ssl_mode       = TLNF_SSL_AUTO,
    .flags          = 0,
};
