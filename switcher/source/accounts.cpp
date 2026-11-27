/*  Talonflame Switcher — per-user, per-network account.dat slots
    Copyright 2026 Talonflame contributors
    SPDX-License-Identifier: GPL-3.0-or-later
*/
#include "accounts.h"
#include "fsutil.h"

#include <coreinit/time.h>
#include <mocha/mocha.h>
#include <nn/act.h>

#include <cstdio>

#define SD_ROOT  "fs:/vol/external01/wiiu/talonflame"
#define MLC_NAME "storage_mlc"
#define ACT_DIR  MLC_NAME ":/usr/save/system/act"

static const char *const kNames[NETWORK_COUNT] = {"Nintendo Network", "Brewtendo Network", "Pretendo Network"};
static const char *const kKeys[NETWORK_COUNT] = {"nintendo", "brewtendo", "pretendo"};

static bool gMochaReady = false;
static bool gMlcMounted = false;
static bool gActReady = false;

const char *networkName(int net) {
    return (net >= 0 && net < NETWORK_COUNT) ? kNames[net] : "Unknown";
}

const char *networkKey(int net) {
    return (net >= 0 && net < NETWORK_COUNT) ? kKeys[net] : "unknown";
}

static std::string pidHex(uint32_t pid) {
    char buf[9];
    snprintf(buf, sizeof(buf), "%08x", (unsigned int)pid);
    return buf;
}

static std::string liveAccountPath(const User &u) {
    return std::string(ACT_DIR "/") + pidHex(u.persistentId) + "/account.dat";
}

static std::string userDir(const User &u) {
    return std::string(SD_ROOT "/accounts/") + pidHex(u.persistentId);
}

static std::string slotPath(const User &u, int net) {
    return userDir(u) + "/" + networkKey(net) + ".dat";
}

static std::string tagPath(const User &u) {
    return userDir(u) + "/current.txt";
}

std::string accountsInit() {
    if (Mocha_InitLibrary() != MOCHA_RESULT_SUCCESS) {
        return "Mocha is not available (is wafel_mocha / MochaPayload loaded?)";
    }
    gMochaReady = true;
    if (Mocha_MountFS(MLC_NAME, nullptr, "/vol/storage_mlc01") != MOCHA_RESULT_SUCCESS) {
        return "could not mount the system storage (MLC)";
    }
    gMlcMounted = true;
    nn::act::Initialize();
    gActReady = true;
    if (!fsutil::makeDirs(SD_ROOT "/accounts") || !fsutil::makeDirs(SD_ROOT "/backups")) {
        return "could not create sd:/wiiu/talonflame";
    }
    return "";
}

void accountsShutdown() {
    if (gActReady) {
        nn::act::Finalize();
    }
    if (gMlcMounted) {
        Mocha_UnmountFS(MLC_NAME);
    }
    if (gMochaReady) {
        Mocha_DeInitLibrary();
    }
    gActReady = gMlcMounted = gMochaReady = false;
}

static std::string miiName(uint8_t slot) {
    int16_t raw[nn::act::MiiNameSize] = {};
    if (nn::act::GetMiiNameEx(raw, slot).IsFailure()) {
        return "?";
    }
    std::string out;
    for (size_t i = 0; i < nn::act::MiiNameSize && raw[i]; i++) {
        uint16_t c = (uint16_t)raw[i];
        out += (c >= 0x20 && c < 0x7F) ? (char)c : '?';
    }
    return out.empty() ? "?" : out;
}

void refreshUser(User &u) {
    for (int n = 0; n < NETWORK_COUNT; n++) {
        u.hasSaved[n] = fsutil::exists(slotPath(u, n));
    }
    u.tag = -1;
    std::string t;
    if (fsutil::readText(tagPath(u), t)) {
        for (int n = 0; n < NETWORK_COUNT; n++) {
            if (t == kKeys[n]) {
                u.tag = n;
            }
        }
    }
}

std::vector<User> enumerateUsers() {
    std::vector<User> users;
    for (uint8_t slot = 1; slot <= 12; slot++) {
        if (!nn::act::IsSlotOccupied(slot)) {
            continue;
        }
        User u;
        u.slot = slot;
        u.persistentId = nn::act::GetPersistentIdEx(slot);
        u.name = miiName(slot);
        u.networkAccount = nn::act::IsNetworkAccountEx(slot);
        refreshUser(u);
        users.push_back(u);
    }
    return users;
}

bool setTag(User &u, int net) {
    if (!fsutil::makeDirs(userDir(u)) || !fsutil::writeText(tagPath(u), networkKey(net))) {
        return false;
    }
    u.tag = net;
    return true;
}

bool saveToSlot(User &u, int net, std::string &err) {
    if (!fsutil::makeDirs(userDir(u))) {
        err = "could not create " + userDir(u);
        return false;
    }
    if (!fsutil::copyFile(liveAccountPath(u), slotPath(u, net))) {
        err = "could not copy account.dat to the SD card";
        return false;
    }
    u.hasSaved[net] = true;
    return true;
}

bool restoreSlot(User &u, int net, std::string &err) {
    std::vector<uint8_t> slotData;
    if (!fsutil::readFile(slotPath(u, net), slotData) || slotData.empty()) {
        err = std::string("no saved ") + networkName(net) + " account for this user";
        return false;
    }

    char backup[128];
    snprintf(backup, sizeof(backup), SD_ROOT "/backups/%s-%llu.dat",
             pidHex(u.persistentId).c_str(), (unsigned long long)OSGetTime());
    if (!fsutil::copyFile(liveAccountPath(u), backup)) {
        err = "safety backup failed, nothing was changed";
        return false;
    }

    if (!fsutil::writeFileVerified(liveAccountPath(u), slotData)) {
        // Put the original back from the safety copy.
        fsutil::copyFile(backup, liveAccountPath(u));
        err = "writing account.dat failed; original restored from backup";
        return false;
    }
    setTag(u, net);
    return true;
}
