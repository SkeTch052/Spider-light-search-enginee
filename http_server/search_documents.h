#pragma once

#include <pqxx/pqxx>
#include <vector>
#include <string>
#include <utility>
#include <iostream>

std::vector<std::pair<std::string, int>> searchDocuments(const std::vector<std::string>& words, pqxx::connection& c);
