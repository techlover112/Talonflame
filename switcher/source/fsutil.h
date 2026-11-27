/*  Talonflame Switcher — file helpers
    Copyright 2026 Talonflame contributors
    SPDX-License-Identifier: GPL-3.0-or-later
*/
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace fsutil {

bool exists(const std::string &path);
bool readFile(const std::string &path, std::vector<uint8_t> &out);
// Writes the whole buffer, then reads it back and compares.
bool writeFileVerified(const std::string &path, const std::vector<uint8_t> &data);
bool copyFile(const std::string &from, const std::string &to);
// mkdir -p; path must not end in a filename.
bool makeDirs(const std::string &path);
bool readText(const std::string &path, std::string &out);
bool writeText(const std::string &path, const std::string &text);

} // namespace fsutil
