#pragma once

#include <string>
#include <vector>
#include <utility>

namespace http_server {

    std::string generateStartPage();
    std::string generateSearchResults(const std::vector<std::string>& query_words, const std::vector<std::pair<std::string, int>>& search_results);
    std::string generateErrorPage(const std::string& error_message);
}
