/*  Talonflame Switcher — per-user, per-network account.dat slots
    Copyright 2026 Talonflame contributors
    SPDX-License-Identifier: GPL-3.0-or-later

    Layout on the SD card:
      sd:/wiiu/talonflame/accounts/<persistentId>/<network>.dat  saved slot
      sd:/wiiu/talonflame/accounts/<persistentId>/current.txt    network the
                                                               live account.dat
                                                               belongs to
      sd:/wiiu/talonflame/backups/<persistentId>-<tick>.dat      safety copy made
                                                               before every write
                                                               to the MLC
*/
#pragma once

#include <cstdint>
#include <string>
#include <vector>

constexpr int NETWORK_COUNT = 3; // matches enum tlnf_profile
const char *networkName(int net);  // "Nintendo Network", ...
const char *networkKey(int net);   // "nintendo", ...

struct User {
    uint8_t slot = 0;
    uint32_t persistentId = 0;
    std::string name;
    bool networkAccount = false;
    int tag = -1; // network the live account.dat belongs to, -1 = unknown
    bool hasSaved[NETWORK_COUNT] = {};
};

// Mounts the MLC through Mocha and initializes nn::act. Returns an error
// string, empty on success.
std::string accountsInit();
void accountsShutdown();

std::vector<User> enumerateUsers();
void refreshUser(User &user);

bool setTag(User &user, int net);
// Copies the live account.dat into the user's slot for `net`.
bool saveToSlot(User &user, int net, std::string &err);
// Safety-backs-up the live account.dat, then writes slot `net` over it and
// retags the user. The change takes effect after a full power cycle.
bool restoreSlot(User &user, int net, std::string &err);
