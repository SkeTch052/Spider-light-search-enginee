#pragma once

#include <string>
#include <stdexcept>
#include "inih-r58/cpp/INIReader.h"

struct Config {
    std::string db_host;
    int db_port;
    std::string db_name;
    std::string db_user;
    std::string db_password;

    std::string start_page;
    int depth;

    int search_port;

    Config() : db_port(0), depth(0), search_port(0) {}
};

Config load_config(const std::string& filename);