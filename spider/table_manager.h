#pragma once

#include <pqxx/pqxx>
#include <stdexcept>

void create_tables(pqxx::connection& c);