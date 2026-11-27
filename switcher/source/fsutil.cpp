/*  Talonflame Switcher — file helpers
    Copyright 2026 Talonflame contributors
    SPDX-License-Identifier: GPL-3.0-or-later
*/
#include "fsutil.h"

#include <cerrno>
#include <cstdio>
#include <sys/stat.h>

namespace fsutil {

bool exists(const std::string &path) {
    struct stat st{};
    return stat(path.c_str(), &st) == 0;
}

bool readFile(const std::string &path, std::vector<uint8_t> &out) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) {
        return false;
    }
    out.clear();
    uint8_t buf[0x1000];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        out.insert(out.end(), buf, buf + n);
    }
    bool ok = !ferror(f);
    fclose(f);
    return ok;
}

bool writeFileVerified(const std::string &path, const std::vector<uint8_t> &data) {
    FILE *f = fopen(path.c_str(), "wb");
    if (!f) {
        return false;
    }
    bool ok = data.empty() || fwrite(data.data(), 1, data.size(), f) == data.size();
    if (fclose(f) != 0) {
        ok = false;
    }
    if (!ok) {
        return false;
    }
    std::vector<uint8_t> check;
    return readFile(path, check) && check == data;
}

bool copyFile(const std::string &from, const std::string &to) {
    std::vector<uint8_t> data;
    return readFile(from, data) && writeFileVerified(to, data);
}

bool makeDirs(const std::string &path) {
    // Skip the device prefix ("fs:/vol/external01") when creating parents.
    size_t start = path.find(":/");
    start = (start == std::string::npos) ? 0 : start + 2;
    for (size_t pos = path.find('/', start); ; pos = path.find('/', pos + 1)) {
        std::string part = (pos == std::string::npos) ? path : path.substr(0, pos);
        if (!part.empty() && !exists(part)) {
            if (mkdir(part.c_str(), 0777) != 0 && errno != EEXIST) {
                return false;
            }
        }
        if (pos == std::string::npos) {
            break;
        }
    }
    return true;
}

bool readText(const std::string &path, std::string &out) {
    std::vector<uint8_t> data;
    if (!readFile(path, data)) {
        return false;
    }
    out.assign(data.begin(), data.end());
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r' || out.back() == ' ')) {
        out.pop_back();
    }
    return true;
}

bool writeText(const std::string &path, const std::string &text) {
    return writeFileVerified(path, std::vector<uint8_t>(text.begin(), text.end()));
}

} // namespace fsutil
