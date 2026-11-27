/*  Talonflame Switcher — minimal OSScreen text UI + GamePad input
    Copyright 2026 Talonflame contributors
    SPDX-License-Identifier: GPL-3.0-or-later
*/
#include "ui.h"

#include <coreinit/cache.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/screen.h>
#include <vpad/input.h>

namespace ui {

static void *sTvBuf = nullptr;
static void *sDrcBuf = nullptr;
static uint32_t sTvSize = 0;
static uint32_t sDrcSize = 0;

bool init() {
    OSScreenInit();
    sTvSize = OSScreenGetBufferSizeEx(SCREEN_TV);
    sDrcSize = OSScreenGetBufferSizeEx(SCREEN_DRC);
    sTvBuf = MEMAllocFromDefaultHeapEx(sTvSize, 0x100);
    sDrcBuf = MEMAllocFromDefaultHeapEx(sDrcSize, 0x100);
    if (!sTvBuf || !sDrcBuf) {
        shutdown();
        return false;
    }
    OSScreenSetBufferEx(SCREEN_TV, sTvBuf);
    OSScreenSetBufferEx(SCREEN_DRC, sDrcBuf);
    OSScreenEnableEx(SCREEN_TV, TRUE);
    OSScreenEnableEx(SCREEN_DRC, TRUE);
    return true;
}

void shutdown() {
    if (sTvBuf || sDrcBuf) {
        OSScreenEnableEx(SCREEN_TV, FALSE);
        OSScreenEnableEx(SCREEN_DRC, FALSE);
        OSScreenShutdown();
    }
    if (sTvBuf) {
        MEMFreeToDefaultHeap(sTvBuf);
    }
    if (sDrcBuf) {
        MEMFreeToDefaultHeap(sDrcBuf);
    }
    sTvBuf = sDrcBuf = nullptr;
}

void begin() {
    OSScreenClearBufferEx(SCREEN_TV, 0x101820FF);
    OSScreenClearBufferEx(SCREEN_DRC, 0x101820FF);
}

void line(int row, const std::string &text) {
    std::string t = text.size() > (size_t)COLS ? text.substr(0, COLS) : text;
    OSScreenPutFontEx(SCREEN_TV, 0, row, t.c_str());
    OSScreenPutFontEx(SCREEN_DRC, 0, row, t.c_str());
}

void end() {
    DCFlushRange(sTvBuf, sTvSize);
    DCFlushRange(sDrcBuf, sDrcSize);
    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);
}

uint32_t pressed() {
    VPADStatus st{};
    VPADReadError err = VPAD_READ_SUCCESS;
    if (VPADRead(VPAD_CHAN_0, &st, 1, &err) < 1 || err != VPAD_READ_SUCCESS) {
        return 0;
    }
    return st.trigger;
}

} // namespace ui
