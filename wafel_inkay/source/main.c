/*  Talonflame — wafel_inkay
    Inkay-style network patches as a stroopwafel plugin (.ipx), applied once
    at IOSU boot under ISFShax. No Aroma is required for these patches.

    Patch data derived from:
      - Inkay  — Copyright 2022-2024 Pretendo Network contributors
      - Bnkay  — Copyright 2026 Brewtendo Network contributors
    Plugin structure follows StroopwafelCFW/stroopwafel's wafel_plugin_example.

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

/* ============================================================================
 * OVERVIEW
 * ============================================================================
 *
 * How this differs from Inkay/Bnkay:
 *
 *   Inkay is an Aroma module. It runs on the PPC side after the system has
 *   booted and uses Mocha's IOSU kernel-write IPC (Mocha_IOSUKernelWrite32)
 *   to poke new values into IOSU's *virtual* address space while IOSU is
 *   already running.
 *
 *   wafel_inkay runs inside IOSU itself. minute (ISFShax's boot payload)
 *   loads fw.img, then stroopwafel's wafel_core calls our kern_main() BEFORE
 *   the MMU is enabled and BEFORE any IOSU module has started. At that point
 *   every fw.img segment sits in memory at its *physical* load address, so:
 *
 *     1. We translate each IOSU virtual address (the same addresses Inkay
 *        uses) to its physical address with the fw.img program headers.
 *     2. We write the new bytes there directly.
 *     3. When IOSU boots, every module starts with our values already in
 *        place — no runtime poking, no race with modules reading the old
 *        values, and no "policy list refresh" poke is needed.
 *
 *   Consequences:
 *     - .bss addresses that Inkay patches at runtime (0xE24B8A24,
 *       0xE31930D4) do not exist yet at boot (bss is zero-filled when the
 *       module starts). We skip them. The policy-list URL is derived from
 *       the patched nppl hostname; the account-API format string is fixed
 *       at its .rodata/.data source via a pattern search instead.
 *     - Changing networks requires a reboot (the switcher app says so).
 *
 * Configuration:
 *
 *   tlnf_config (tlnf_config.c) is a tagged struct in this plugin's .data.
 *   The Talonflame switcher app rewrites it inside wafel_inkay.ipx on the SD
 *   card. We just read it here.
 *
 * Patch summary by profile:
 *
 *                       URL patches     account URL     SSL verify patch
 *     NINTENDO          none            none            applied
 *     BREWTENDO         Brewtendo set   brewtendo.org   applied
 *     PRETENDO          Pretendo set    pretendo.cc     applied
 *
 *   The SSL patch is identical for all three profiles (one 32-bit write),
 *   unless TLNF_FLAG_NO_SSL_PATCH is set in the config.
 *
 * Firmware:
 *
 *   ISFShax only supports 5.5.x. Addresses are for 5.5.x. wafel_core sets
 *   its own is_55x flag; we repeat the cheap MCP-entrypoint check and bail
 *   out entirely if it does not match, so a wrong firmware is never
 *   patched. The 5.5.5+ vs 5.5.0-5.5.4 SSL patch site is selected by
 *   tlnf_config.ssl_mode, which the switcher app fills in from
 *   MCP_GetSystemVersion() (IOSU cannot easily ask that pre-MMU).
 *
 * TVii:
 *
 *   Intentionally not handled here; it is patched by a separate app.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <elf.h>
#include <wafel/dynamic.h>
#include <wafel/ios_dynamic.h>
#include <wafel/utils.h>
#include <wafel/bm.h>

#include "tlnf_config.h"
#include "url_patches.h"

/* Longest URL/string we ever write, including NUL, rounded up. */
#define MAX_PATCH_STR 96

/* MCP entrypoint on every 5.5.x fw.img (same check wafel_core performs). */
#define MCP_ENTRYPOINT_55X 0x05056718u

#define LOG(...) debug_printf("wafel_inkay: " __VA_ARGS__)

/* ============================================================================
 * SECTION 1 — Address translation (pre-MMU)
 * ============================================================================
 *
 * vaddr_to_paddr(vaddr) -> physical address, or 0 on failure.
 *
 *   Walk the fw.img program headers that wafel_core keeps at 0x1D000000
 *   (phdr table at index IOS_SEG_PHDRS, real segments start at
 *   IOS_SEG_ELFINFO). For the segment where
 *       p_vaddr <= vaddr < p_vaddr + p_memsz
 *   return vaddr - p_vaddr + p_paddr.
 *
 *   Additionally require vaddr + len <= p_vaddr + p_filesz, so we never
 *   "patch" a .bss tail that the loader will zero later.
 *
 *   We do this ourselves (instead of wafel's ios_elf_vaddr_to_paddr) because
 *   that helper returns the input address on failure, and writing to an
 *   untranslated 0xE... address pre-MMU would scribble over random memory.
 */
static uintptr_t vaddr_to_paddr(uint32_t vaddr, uint32_t len)
{
    Elf32_Phdr *phdrs = (Elf32_Phdr *)0x1D000000;
    uint32_t num_phdrs = phdrs[IOS_SEG_PHDRS].p_memsz / sizeof(Elf32_Phdr);

    for (uint32_t i = IOS_SEG_ELFINFO; i < num_phdrs; i++) {
        uint32_t start = phdrs[i].p_vaddr;
        if (vaddr < start || vaddr - start >= phdrs[i].p_memsz) {
            continue;
        }
        uint32_t off = vaddr - start;
        if (len > phdrs[i].p_filesz || off > phdrs[i].p_filesz - len) {
            LOG("%08lx+%lu is outside the file-backed part of its segment\n",
                (unsigned long)vaddr, (unsigned long)len);
            return 0;
        }
        return (uintptr_t)(phdrs[i].p_paddr + off);
    }

    LOG("no segment contains %08lx\n", (unsigned long)vaddr);
    return 0;
}

/* ============================================================================
 * SECTION 2 — Primitive writers
 * ============================================================================
 *
 * write_string(vaddr, str)
 *   - Translate with vaddr_to_paddr(vaddr, len).
 *   - Copy str (with NUL) into a zeroed stack buffer, round the length up to
 *     an even number, and memcpy16() it to the physical address. memcpy16
 *     keeps writes 2-byte aligned, which is what wafel_core's own patch
 *     helpers use. The extra pad byte is zero and lands in the original
 *     string's alignment padding.
 *   - Return 0 on success, -1 on failure (logged).
 *
 * write_u32(vaddr, value)
 *   - Translate, then memcpy16() the 4 bytes. Same return convention.
 *
 * read_u32(vaddr)
 *   - Translate and read32(); returns 0 if translation fails. Used only for
 *     logging / "already patched" checks.
 *
 * No cache maintenance is needed: we run pre-MMU with caches off for this
 * region, and wafel_core invalidates the I-cache after all plugins return.
 */
static int write_string(uint32_t vaddr, const char *str)
{
    char buf[MAX_PATCH_STR];
    size_t len = strlen(str) + 1;
    size_t even = (len + 1) & ~(size_t)1;

    if (even > sizeof(buf)) {
        LOG("string too long for %08lx\n", (unsigned long)vaddr);
        return -1;
    }

    uintptr_t pa = vaddr_to_paddr(vaddr, even);
    if (!pa) {
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    memcpy(buf, str, len);
    memcpy16((void *)pa, buf, even);
    return 0;
}

static int write_u32(uint32_t vaddr, uint32_t value)
{
    uintptr_t pa = vaddr_to_paddr(vaddr, sizeof(value));
    if (!pa) {
        return -1;
    }
    memcpy16((void *)pa, &value, sizeof(value));
    return 0;
}

static uint32_t read_u32(uint32_t vaddr)
{
    uintptr_t pa = vaddr_to_paddr(vaddr, sizeof(uint32_t));
    if (!pa) {
        return 0;
    }
    uint32_t value;
    memcpy16(&value, (const void *)pa, sizeof(value));
    return value;
}

/* ============================================================================
 * SECTION 3 — Pattern search & replace across fw.img
 * ============================================================================
 *
 * replace_string_everywhere(orig, repl) -> number of replacements, -1 on error
 *
 *   - Search pattern = orig INCLUDING its NUL, so only whole strings match.
 *   - "Slot" size = strlen(orig)+1 rounded up to 4 (strings in .rodata are
 *     4-byte aligned, so the bytes up to the next multiple of 4 are padding).
 *   - Refuse if strlen(repl)+1 > slot.
 *   - For each loaded segment (index >= IOS_SEG_ELFINFO, p_filesz > 0):
 *       boyer_moore_search() over [p_paddr, p_paddr + p_filesz), and for
 *       every hit, memcpy16() the zero-padded replacement over `slot` bytes,
 *       then continue searching after the hit.
 *   - Log the count. Zero hits is not fatal (just logged).
 */
static int replace_string_everywhere(const char *orig, const char *repl)
{
    char buf[MAX_PATCH_STR];
    uint32_t patlen = strlen(orig) + 1; /* include NUL: whole strings only */
    uint32_t replen = strlen(repl) + 1;
    uint32_t slot = (patlen + 3) & ~3u;

    if (replen > slot || slot > sizeof(buf)) {
        LOG("'%s' does not fit in the slot of '%s'\n", repl, orig);
        return -1;
    }
    memset(buf, 0, sizeof(buf));
    memcpy(buf, repl, replen);

    Elf32_Phdr *phdrs = (Elf32_Phdr *)0x1D000000;
    uint32_t num_phdrs = phdrs[IOS_SEG_PHDRS].p_memsz / sizeof(Elf32_Phdr);
    int count = 0;

    for (uint32_t i = IOS_SEG_ELFINFO; i < num_phdrs; i++) {
        u8 *cur = (u8 *)phdrs[i].p_paddr;
        u8 *end = cur + phdrs[i].p_filesz;

        while ((uint32_t)(end - cur) >= patlen) {
            u8 *hit = boyer_moore_search(cur, (int)(end - cur), (void *)orig, (int)patlen);
            if (!hit) {
                break;
            }
            /* only overwrite the padding if it is still inside the segment
             * and 2-byte aligned for memcpy16 */
            uint32_t n = ((uint32_t)(end - hit) >= slot) ? slot : ((replen + 1) & ~1u);
            if (((uintptr_t)hit & 1) || (uint32_t)(end - hit) < n) {
                LOG("skipping unaligned/truncated match at %p\n", hit);
            } else {
                memcpy16(hit, buf, n);
                count++;
            }
            cur = hit + patlen;
        }
    }

    LOG("replaced %d occurrence(s) of '%s'\n", count, orig);
    return count;
}

/* ============================================================================
 * SECTION 4 — Firmware sanity check
 * ============================================================================
 *
 * firmware_is_55x()
 *   - ios_elf_get_module_entrypoint(IOS_MCP) == MCP_ENTRYPOINT_55X.
 *   - If false, kern_main logs and returns without patching anything.
 */
static int firmware_is_55x(void)
{
    return ios_elf_get_module_entrypoint(IOS_MCP) == MCP_ENTRYPOINT_55X;
}

/* ============================================================================
 * SECTION 5 — Config validation
 * ============================================================================
 *
 * config_valid()
 *   - struct_version == 1, profile in range, ssl_mode in range. On failure:
 *     log and fall back to the Nintendo profile (no URL patches); the SSL
 *     patch is still applied, as it is for every profile.
 */
static int config_valid(void)
{
    /* The magic is deliberately not compared here: a second copy of it in
     * .rodata would make the switcher app's search of the .ipx ambiguous. */
    return tlnf_config.struct_version == 1 &&
           tlnf_config.profile <= TLNF_PROFILE_PRETENDO &&
           tlnf_config.ssl_mode <= TLNF_SSL_LEGACY;
}

/* ============================================================================
 * SECTION 6 — Network URL patches
 * ============================================================================
 *
 * apply_network_patches(profile)
 *   NINTENDO  -> nothing.
 *   BREWTENDO -> write_string() every entry of url_patches_brewtendo,
 *                then replace_string_everywhere(ACCOUNT_URL_ORIG,
 *                                               ACCOUNT_URL_BREWTENDO).
 *   PRETENDO  -> same with url_patches_pretendo / ACCOUNT_URL_PRETENDO.
 *   Count and log failures; a failed entry does not stop the others.
 */
#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

static void apply_network_patches(uint32_t profile)
{
    const url_patch *table;
    unsigned int count;
    const char *account_url;
    const char *name;

    switch (profile) {
    case TLNF_PROFILE_BREWTENDO:
        table = url_patches_brewtendo;
        count = ARRAY_LEN(url_patches_brewtendo);
        account_url = ACCOUNT_URL_BREWTENDO;
        name = "Brewtendo";
        break;
    case TLNF_PROFILE_PRETENDO:
        table = url_patches_pretendo;
        count = ARRAY_LEN(url_patches_pretendo);
        account_url = ACCOUNT_URL_PRETENDO;
        name = "Pretendo";
        break;
    default:
        LOG("profile Nintendo: leaving URLs untouched\n");
        return;
    }

    unsigned int failed = 0;
    for (unsigned int i = 0; i < count; i++) {
        if (write_string(table[i].address, table[i].url) != 0) {
            failed++;
        }
    }
    LOG("%s: %u/%u URL patches applied\n", name, count - failed, count);

    replace_string_everywhere(ACCOUNT_URL_ORIG, account_url);
}

/* ============================================================================
 * SECTION 7 — SSL certificate-verification patch (IOS-NET / NSSL)
 * ============================================================================
 *
 * apply_ssl_patch()
 *   Equivalent of Inkay's runtime block:
 *
 *       if (is555(get_console_os_version()))
 *           Mocha_IOSUKernelWrite32(0xE1019F78, 0xE3A00001); // mov r0, #1
 *       else
 *           Mocha_IOSUKernelWrite32(0xE1019E84, 0xE3A00001); // mov r0, #1
 *
 *   Here:
 *     - Pick NSSL_VERIFY_PATCH_ADDR_555 or _LEGACY from tlnf_config.ssl_mode.
 *     - Skip if TLNF_FLAG_NO_SSL_PATCH is set.
 *     - Log the original word, skip if it already equals the patch value,
 *       otherwise write_u32(addr, NSSL_VERIFY_PATCH_VALUE).
 *   Applied for ALL profiles, including Nintendo.
 */
static void apply_ssl_patch(void)
{
    if (tlnf_config.flags & TLNF_FLAG_NO_SSL_PATCH) {
        LOG("SSL patch disabled by config\n");
        return;
    }

    uint32_t addr = (tlnf_config.ssl_mode == TLNF_SSL_LEGACY)
                        ? NSSL_VERIFY_PATCH_ADDR_LEGACY
                        : NSSL_VERIFY_PATCH_ADDR_555;

    uint32_t orig = read_u32(addr);
    LOG("SSL patch site %08lx holds %08lx\n", (unsigned long)addr, (unsigned long)orig);

    if (orig == NSSL_VERIFY_PATCH_VALUE) {
        LOG("SSL patch already present\n");
        return;
    }
    if (write_u32(addr, NSSL_VERIFY_PATCH_VALUE) == 0) {
        LOG("SSL patch applied\n");
    }
}

/* ============================================================================
 * SECTION 8 — stroopwafel entry points
 * ============================================================================
 *
 * kern_main()  — runs pre-MMU in kernel mode, before every IOSU module.
 *   1. Log version + config.
 *   2. firmware_is_55x() or return.
 *   3. config_valid() or use the safe default.
 *   4. apply_network_patches(profile).
 *   5. apply_ssl_patch().
 *   6. Return (wafel_core continues boot and invalidates caches).
 *
 * mcp_main()   — runs before MCP's main thread. Nothing to do; all patches
 *   are applied in kern_main. Kept because wafel_core calls it.
 */
__attribute__((target("arm")))
void kern_main(void)
{
    LOG("starting (config at %p)\n", (void *)&tlnf_config);

    if (!firmware_is_55x()) {
        LOG("not a 5.5.x fw.img, applying nothing\n");
        return;
    }

    uint32_t profile = TLNF_PROFILE_NINTENDO;
    if (config_valid()) {
        profile = tlnf_config.profile;
    } else {
        LOG("config block invalid, falling back to Nintendo profile\n");
    }
    LOG("profile=%lu ssl_mode=%lu flags=%08lx\n", (unsigned long)profile,
        (unsigned long)tlnf_config.ssl_mode, (unsigned long)tlnf_config.flags);

    apply_network_patches(profile);
    apply_ssl_patch();

    LOG("done\n");
}

void mcp_main(void)
{
}
