//
// Created by ash on 17/12/24.
//

#ifndef BNKAY_BNKAY_CONFIG_H
#define BNKAY_BNKAY_CONFIG_H

#ifdef __has_include
#if __has_include("bnkay_config.local.h")

#include "bnkay_config.local.h"
#define BNKAY_CUSTOM 1

#endif
#endif

#ifndef NETWORK_BASEURL
#define NETWORK_BASEURL "pretendo.cc"
#endif

#endif //BNKAY_BNKAY_CONFIG_H
