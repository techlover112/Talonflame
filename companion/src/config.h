//
// Created by ash on 10/12/22.
//

#ifndef BNKAY_CONFIG_H
#define BNKAY_CONFIG_H

class Config {
public:

    static bool connect_to_network;

    /*
     * this allows us to either let pretendo urls,
     * or just let our urls take over,
     * its easier since well id rather let pretendo work as normal
     */
    static bool connect_to_brewtendo;

    static bool enable_tvii;

    static bool initialized;

    static bool shown_warning;

    static bool plugin_is_loaded;

    static bool block_initialize;
};

#endif //BNKAY_CONFIG_H
