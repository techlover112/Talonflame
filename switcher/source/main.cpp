/*  Talonflame Switcher — Aroma app
    Switches the network wafel_inkay patches the console to at boot
    (Nintendo / Brewtendo / Pretendo), and swaps each user's account.dat for
    the matching network.

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

#include "accounts.h"
#include "ipx_config.h"
#include "tlnf_config.h"
#include "ui.h"

#include <coreinit/mcp.h>
#include <vpad/input.h>
#include <whb/proc.h>

#include <cstdio>
#include <string>
#include <vector>

#define APP_VERSION "v1.0.0"

namespace {

enum class Screen { Tagging, Main, Accounts, User, Result, Fatal };

struct State {
    Screen screen = Screen::Main;
    IpxConfig ipx;
    uint32_t wantedSslMode = TLNF_SSL_AUTO;
    std::string fwString;
    std::vector<User> users;
    int cursor = 0;
    int selNet = TLNF_PROFILE_BREWTENDO;
    int selUser = 0;
    int tagIndex = -1; // user being asked about on the Tagging screen
    std::vector<std::string> messages;
    Screen afterResult = Screen::Main;
};

int wrap(int v, int n) {
    return ((v % n) + n) % n;
}

void detectFirmware(State &s) {
    MCPSystemVersion ver{};
    int32_t h = MCP_Open();
    bool ok = h >= 0 && MCP_GetSystemVersion(h, &ver) == 0;
    if (h >= 0) {
        MCP_Close(h);
    }
    if (!ok) {
        s.fwString = "unknown";
        s.wantedSslMode = TLNF_SSL_AUTO;
        return;
    }
    s.fwString = std::to_string(ver.major) + "." + std::to_string(ver.minor) + "." +
                 std::to_string(ver.patch) + ver.region;
    bool is555plus = ver.major == 5 && ver.minor == 5 && ver.patch >= 5;
    s.wantedSslMode = is555plus ? TLNF_SSL_AUTO : TLNF_SSL_LEGACY;
}

// Users whose live account.dat network is unknown. Unlinked users work on any
// network, so they are tagged automatically with the current profile.
int nextUntagged(State &s) {
    int fallback = s.ipx.found ? (int)s.ipx.profile : TLNF_PROFILE_NINTENDO;
    for (size_t i = 0; i < s.users.size(); i++) {
        User &u = s.users[i];
        if (u.tag >= 0) {
            continue;
        }
        if (!u.networkAccount) {
            setTag(u, fallback);
            continue;
        }
        return (int)i;
    }
    return -1;
}

void showResult(State &s, std::vector<std::string> msgs, Screen after) {
    s.messages = std::move(msgs);
    s.afterResult = after;
    s.screen = Screen::Result;
}

void applyNetwork(State &s, int target) {
    std::vector<std::string> out;

    s.ipx = ipxLoad();
    if (!s.ipx.found) {
        showResult(s, {"Nothing was changed:", s.ipx.error}, Screen::Main);
        return;
    }

    for (User &u : s.users) {
        std::string who = u.name + ": ";
        std::string err;

        if (u.tag == target) {
            out.push_back(who + "already set up for " + networkName(target));
            continue;
        }
        if (u.tag < 0) {
            out.push_back(who + "network unknown, skipped");
            continue;
        }
        if (!saveToSlot(u, u.tag, err)) {
            out.push_back(who + "backup failed (" + err + "), skipped");
            continue;
        }
        if (u.hasSaved[target]) {
            if (restoreSlot(u, target, err)) {
                out.push_back(who + "switched to saved " + networkName(target) + " account");
            } else {
                out.push_back(who + err);
            }
        } else if (!u.networkAccount) {
            setTag(u, target);
            out.push_back(who + "not linked, works on any network");
        } else {
            out.push_back(who + "no saved " + std::string(networkName(target)) + " account;");
            out.push_back("    kept current one (it will not sign in there)");
        }
    }

    if (!ipxSave(s.ipx, (uint32_t)target, s.wantedSslMode)) {
        out.insert(out.begin(), "Network NOT changed: " + s.ipx.error);
    } else {
        out.insert(out.begin(), std::string("Network set to ") + networkName(target) + ".");
        out.push_back("");
        out.push_back("Power the console OFF and back ON to apply.");
    }
    showResult(s, out, Screen::Main);
}

void drawHeader(const State &s, const char *title) {
    ui::line(0, std::string("Talonflame Switcher " APP_VERSION " - ") + title);
    std::string net = s.ipx.found ? networkName((int)s.ipx.profile) : "plugin not found";
    ui::line(1, "Boot network: " + net + "   FW: " + s.fwString);
    ui::line(2, "------------------------------------------------------------------");
}

void drawCursor(int row, bool selected, const std::string &text) {
    ui::line(row, std::string(selected ? " > " : "   ") + text);
}

void frameTagging(State &s, uint32_t btn) {
    User &u = s.users[s.tagIndex];
    if (btn & VPAD_BUTTON_LEFT) s.selNet = wrap(s.selNet - 1, NETWORK_COUNT);
    if (btn & VPAD_BUTTON_RIGHT) s.selNet = wrap(s.selNet + 1, NETWORK_COUNT);
    if (btn & VPAD_BUTTON_A) {
        setTag(u, s.selNet);
        s.tagIndex = nextUntagged(s);
        if (s.tagIndex < 0) {
            s.screen = Screen::Main;
        }
        return;
    }

    ui::begin();
    drawHeader(s, "First-time setup");
    ui::line(4, "Which network is this user's linked account on?");
    ui::line(6, "   User: " + u.name);
    ui::line(8, std::string("   < ") + networkName(s.selNet) + " >");
    ui::line(11, "This is asked once per user so the right account.dat is");
    ui::line(12, "saved when you switch networks.");
    ui::line(15, "Left/Right: choose   A: confirm");
    ui::end();
}

void frameMain(State &s, uint32_t btn) {
    const int items = 4;
    if (btn & VPAD_BUTTON_UP) s.cursor = wrap(s.cursor - 1, items);
    if (btn & VPAD_BUTTON_DOWN) s.cursor = wrap(s.cursor + 1, items);
    if (s.cursor == 0) {
        if (btn & VPAD_BUTTON_LEFT) s.selNet = wrap(s.selNet - 1, NETWORK_COUNT);
        if (btn & VPAD_BUTTON_RIGHT) s.selNet = wrap(s.selNet + 1, NETWORK_COUNT);
    }
    if (btn & VPAD_BUTTON_A) {
        switch (s.cursor) {
        case 1: applyNetwork(s, s.selNet); return;
        case 2: s.screen = Screen::Accounts; s.cursor = 0; return;
        case 3: WHBProcStopRunning(); return;
        default: break;
        }
    }

    ui::begin();
    drawHeader(s, "Main");
    drawCursor(4, s.cursor == 0, std::string("Network: < ") + networkName(s.selNet) + " >");
    drawCursor(5, s.cursor == 1, "Apply network (and swap accounts)");
    drawCursor(6, s.cursor == 2, "Manage accounts");
    drawCursor(7, s.cursor == 3, "Exit");
    if (!s.ipx.found) {
        ui::line(10, "Warning: " + s.ipx.error);
    } else if (s.ipx.ssl_mode != s.wantedSslMode) {
        ui::line(10, "Note: SSL patch site does not match this firmware;");
        ui::line(11, "      Apply will correct it.");
    }
    ui::line(15, "Up/Down: move   Left/Right: change   A: select");
    ui::end();
}

void frameAccounts(State &s, uint32_t btn) {
    int items = (int)s.users.size() + 1;
    if (btn & VPAD_BUTTON_UP) s.cursor = wrap(s.cursor - 1, items);
    if (btn & VPAD_BUTTON_DOWN) s.cursor = wrap(s.cursor + 1, items);
    if (btn & VPAD_BUTTON_B) { s.screen = Screen::Main; s.cursor = 2; return; }
    if (btn & VPAD_BUTTON_A) {
        if (s.cursor == items - 1) { s.screen = Screen::Main; s.cursor = 2; return; }
        s.selUser = s.cursor;
        s.selNet = s.users[s.selUser].tag >= 0 ? s.users[s.selUser].tag : 0;
        s.cursor = 0;
        s.screen = Screen::User;
        return;
    }

    ui::begin();
    drawHeader(s, "Accounts");
    ui::line(3, "   User          Live account     Saved: N B P");
    int row = 4;
    for (size_t i = 0; i < s.users.size() && row < 14; i++, row++) {
        const User &u = s.users[i];
        char buf[80];
        snprintf(buf, sizeof(buf), "%-12.12s  %-17.17s       %c %c %c", u.name.c_str(),
                 u.networkAccount ? networkName(u.tag) : "(not linked)",
                 u.hasSaved[0] ? '*' : '-', u.hasSaved[1] ? '*' : '-', u.hasSaved[2] ? '*' : '-');
        drawCursor(row, s.cursor == (int)i, buf);
    }
    drawCursor(row, s.cursor == items - 1, "Back");
    ui::line(15, "A: select user   B: back");
    ui::end();
}

void frameUser(State &s, uint32_t btn) {
    User &u = s.users[s.selUser];
    const int items = 5;
    if (btn & VPAD_BUTTON_UP) s.cursor = wrap(s.cursor - 1, items);
    if (btn & VPAD_BUTTON_DOWN) s.cursor = wrap(s.cursor + 1, items);
    if (s.cursor == 0) {
        if (btn & VPAD_BUTTON_LEFT) s.selNet = wrap(s.selNet - 1, NETWORK_COUNT);
        if (btn & VPAD_BUTTON_RIGHT) s.selNet = wrap(s.selNet + 1, NETWORK_COUNT);
    }
    if (btn & VPAD_BUTTON_B) { s.screen = Screen::Accounts; s.cursor = s.selUser; return; }
    if (btn & VPAD_BUTTON_A) {
        std::string err;
        std::string net = networkName(s.selNet);
        switch (s.cursor) {
        case 1:
            if (setTag(u, s.selNet)) {
                showResult(s, {"Live account marked as " + net + "."}, Screen::User);
            } else {
                showResult(s, {"Could not write to the SD card."}, Screen::User);
            }
            return;
        case 2:
            if (saveToSlot(u, s.selNet, err)) {
                showResult(s, {"Live account saved to the " + net + " slot."}, Screen::User);
            } else {
                showResult(s, {"Save failed: " + err}, Screen::User);
            }
            return;
        case 3:
            if (restoreSlot(u, s.selNet, err)) {
                showResult(s, {"Restored the saved " + net + " account.",
                               "A safety copy of the old one is in",
                               "sd:/wiiu/talonflame/backups/.",
                               "",
                               "Power the console OFF and back ON to apply."},
                           Screen::User);
            } else {
                showResult(s, {"Restore failed: " + err}, Screen::User);
            }
            return;
        case 4:
            s.screen = Screen::Accounts;
            s.cursor = s.selUser;
            return;
        default:
            break;
        }
    }

    ui::begin();
    drawHeader(s, "User");
    ui::line(3, "   User: " + u.name + (u.networkAccount ? "" : " (not linked)"));
    ui::line(4, std::string("   Live account is for: ") + networkName(u.tag));
    drawCursor(6, s.cursor == 0, "Slot: < " + std::string(networkName(s.selNet)) +
                                     (u.hasSaved[s.selNet] ? " > (saved)" : " > (empty)"));
    drawCursor(7, s.cursor == 1, "Mark live account as this network");
    drawCursor(8, s.cursor == 2, "Save live account to this slot");
    drawCursor(9, s.cursor == 3, "Restore this slot as the live account");
    drawCursor(10, s.cursor == 4, "Back");
    ui::line(15, "Up/Down: move   Left/Right: slot   A: select   B: back");
    ui::end();
}

void frameResult(State &s, uint32_t btn) {
    if (btn & (VPAD_BUTTON_A | VPAD_BUTTON_B)) {
        s.screen = s.afterResult;
        return;
    }
    ui::begin();
    drawHeader(s, "Result");
    int row = 4;
    for (const auto &m : s.messages) {
        if (row >= 15) break;
        ui::line(row++, m);
    }
    ui::line(15, "A: continue");
    ui::end();
}

void frameFatal(State &s, uint32_t btn) {
    if (btn & (VPAD_BUTTON_A | VPAD_BUTTON_B | VPAD_BUTTON_HOME)) {
        WHBProcStopRunning();
        return;
    }
    ui::begin();
    drawHeader(s, "Error");
    int row = 4;
    for (const auto &m : s.messages) {
        ui::line(row++, m);
    }
    ui::line(15, "A: exit");
    ui::end();
}

} // namespace

int main(int argc, char **argv) {
    WHBProcInit();
    if (!ui::init()) {
        WHBProcShutdown();
        return 0;
    }

    State s;
    detectFirmware(s);
    s.ipx = ipxLoad();
    if (s.ipx.found) {
        s.selNet = (int)s.ipx.profile;
    }

    std::string err = accountsInit();
    if (!err.empty()) {
        s.messages = {"Could not start:", err};
        s.screen = Screen::Fatal;
    } else {
        s.users = enumerateUsers();
        s.tagIndex = nextUntagged(s);
        if (s.tagIndex >= 0) {
            s.screen = Screen::Tagging;
        }
    }

    while (WHBProcIsRunning()) {
        uint32_t btn = ui::pressed();
        switch (s.screen) {
        case Screen::Tagging:  frameTagging(s, btn); break;
        case Screen::Main:     frameMain(s, btn); break;
        case Screen::Accounts: frameAccounts(s, btn); break;
        case Screen::User:     frameUser(s, btn); break;
        case Screen::Result:   frameResult(s, btn); break;
        case Screen::Fatal:    frameFatal(s, btn); break;
        }
    }

    accountsShutdown();
    ui::shutdown();
    WHBProcShutdown();
    return 0;
}
