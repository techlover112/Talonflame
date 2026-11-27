/*  Talonflame — wafel_inkay embedded configuration block.

    The config lives in the plugin's .data section, tagged with a magic
    string. The Talonflame switcher app edits the .ipx file on the SD card
    in place (it searches for the magic and rewrites the fields), so the
    selected profile persists and is applied at the next boot by minute.

    Copyright 2026 Talonflame contributors
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TLNF_CONFIG_H
#define TLNF_CONFIG_H

#include <stdint.h>

#define TLNF_MAGIC "TLNFCFG1"
#define TLNF_MAGIC_LEN 8

enum tlnf_profile {
    TLNF_PROFILE_NINTENDO  = 0, /* no patches at all */
    TLNF_PROFILE_BREWTENDO = 1,
    TLNF_PROFILE_PRETENDO  = 2,
};

enum tlnf_ssl_mode {
    TLNF_SSL_AUTO   = 0, /* assume FW >= 5.5.5 (every ISFShax console in practice) */
    TLNF_SSL_LEGACY = 1, /* FW 5.5.0 - 5.5.4 */
};

#define TLNF_FLAG_NO_SSL_PATCH (1u << 0) /* skip the NSSL verify bypass */

typedef struct {
    char     magic[TLNF_MAGIC_LEN]; /* "TLNFCFG1", NOT NUL-terminated */
    uint32_t struct_version;        /* 1 */
    uint32_t profile;               /* enum tlnf_profile */
    uint32_t ssl_mode;              /* enum tlnf_ssl_mode */
    uint32_t flags;                 /* TLNF_FLAG_* */
} tlnf_config_t;

/* File-offset layout (all fields big-endian in the .ipx file):
 *   +0x00 magic
 *   +0x08 struct_version
 *   +0x0C profile
 *   +0x10 ssl_mode
 *   +0x14 flags
 */

extern volatile tlnf_config_t tlnf_config;

#endif /* TLNF_CONFIG_H */
