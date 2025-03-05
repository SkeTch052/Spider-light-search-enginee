#include "ezParser.h"

Config load_config(const std::string& filename) {
    INIReader reader(filename);
    Config config;

    if (reader.ParseError() < 0) {
        throw std::runtime_error("Error loading configuration file: " + filename);
    }

    config.db_host = reader.Get("database", "host", "localhost");
    config.db_port = reader.GetInteger("database", "port", 5432);
    config.db_name = reader.Get("database", "dbname", "your_database");
    config.db_user = reader.Get("database", "user", "user");
    config.db_password = reader.Get("database", "password", "password");

    config.depth = reader.GetInteger("spider", "depth", 2);

    config.start_page = reader.Get("search", "start_page", "http://localhost:8081");
    config.search_port = reader.GetInteger("search", "port", 8081);

    return config;
}