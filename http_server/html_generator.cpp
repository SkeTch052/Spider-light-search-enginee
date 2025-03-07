#include "html_generator.h"
#include <sstream>

namespace http_server {

    std::string generateStartPage() {
        std::ostringstream oss;
        oss << "<html>\n"
            << "<head>\n"
            << "    <meta charset=\"UTF-8\">\n"
            << "    <title>Search Engine</title>\n"
            << "    <style>\n"
            << "        body {\n"
            << "            display: flex;\n"
            << "            justify-content: center;\n"
            << "            align-items: center;\n"
            << "            height: 100vh;\n"
            << "            margin-top: -10vh;\n"
            << "            flex-direction: column;\n"
            << "        }\n"
            << "        h1 {\n"
            << "            margin-bottom: 20px;\n"
            << "        }\n"
            << "        form {\n"
            << "            text-align: center;\n"
            << "        }\n"
            << "        input[type=\"text\"] {\n"
            << "            padding: 10px;\n"
            << "            font-size: 16px;\n"
            << "            width: 300px;\n"
            << "            height: 30px;\n"
            << "            margin-bottom: 10px;\n"
            << "            box-sizing: border-box;\n"
            << "        }\n"
            << "        input[type=\"submit\"] {\n"
            << "            font-size: 16px;\n"
            << "            height: 30px;\n"
            << "            cursor: pointer;\n"
            << "            box-sizing: border-box;\n"
            << "        }\n"
            << "    </style>\n"
            << "</head>\n"
            << "<body>\n"
            << "    <h1>Search Engine</h1>\n"
            << "    <p>Welcome!</p>\n"
            << "    <p>Enter 1-4 words, each 3-32 characters long.</p>\n"
            << "    <form action=\"/\" method=\"post\">\n"
            << "        <input type=\"text\" id=\"search\" name=\"search\" maxlength=\"131\"><br>\n"
            << "        <input type=\"submit\" value=\"Search\">\n"
            << "    </form>\n"
            << "</body>\n"
            << "</html>\n";
        return oss.str();
    }

    std::string generateSearchResults(const std::vector<std::string>& query_words,
        const std::vector<std::pair<std::string, int>>& search_results) {
        std::ostringstream oss;

        // Собираем строку из слов запроса
        std::string query;
        for (size_t i = 0; i < query_words.size(); ++i) {
            query += query_words[i];
            if (i < query_words.size() - 1) {
                query += " ";
            }
        }

        oss << "<html>\n"
            << "<head>\n"
            << "    <meta charset=\"UTF-8\">\n"
            << "    <title>Search Engine</title>\n"
            << "    <style>\n"
            << "        body {\n"
            << "            display: flex;\n"
            << "            justify-content: center;\n"
            << "            align-items: center;\n"
            << "            height: 100vh;\n"
            << "            margin-top: -10vh;\n"
            << "            flex-direction: column;\n"
            << "        }\n"
            << "        h1 {\n"
            << "            margin-bottom: 20px;\n"
            << "        }\n"
            << "        p {\n"
            << "            margin-bottom: 10px;\n"
            << "        }\n"
            << "        ul {\n"
            << "            list-style-type: none;\n"
            << "            padding: 0;\n"
            << "            text-align: left;\n"
            << "        }\n"
            << "        li {\n"
            << "            margin: 15px 0;\n"
            << "        }\n"
            << "        a {\n"
            << "            text-decoration: none;\n"
            << "            color: #0078d4;\n"
            << "        }\n"
            << "        a:hover {\n"
            << "            text-decoration: underline;\n"
            << "        }\n"
            << "        form {\n"
            << "            text-align: center;\n"
            << "            margin-top: 20px;\n"
            << "        }\n"
            << "        input[type=\"submit\"] {\n"
            << "            font-size: 16px;\n"
            << "            height: 30px;\n"
            << "            cursor: pointer;\n"
            << "            box-sizing: border-box;\n"
            << "        }\n"
            << "    </style>\n"
            << "</head>\n"
            << "<body>\n"
            << "    <h1>Search Engine</h1>\n"
            << "    <p>Search results for: " << query << "</p>\n"
            << "    <ul>\n";

        if (search_results.empty()) {
            oss << "    <li>No results for this query.</li>\n";
        }
        else {
            for (const auto& result : search_results) {
                oss << "    <li><a href=\"" << result.first << "\">" << result.first
                    << "</a> (Relevance: " << result.second << ")</li>\n";
            }
        }

        oss << "    </ul>\n"
            << "    <form action=\"back\" method=\"post\">\n"
            << "        <input type=\"submit\" value=\"Back to Search\">\n"
            << "    </form>\n"
            << "</body>\n"
            << "</html>\n";
        return oss.str();
    }

    std::string generateErrorPage(const std::string& error_message) {
        std::ostringstream oss;
        oss << "<html>\n"
            << "<head>\n"
            << "    <meta charset=\"UTF-8\">\n"
            << "    <title>Server Error</title>\n"
            << "    <style>\n"
            << "        body {\n"
            << "            display: flex;\n"
            << "            justify-content: center;\n"
            << "            align-items: center;\n"
            << "            height: 100vh;\n"
            << "            margin-top: -10vh;\n"
            << "            flex-direction: column;\n"
            << "        }\n"
            << "        h1 {\n"
            << "            margin-bottom: 20px;\n"
            << "        }\n"
            << "        p {\n"
            << "            margin-bottom: 10px;\n"
            << "        }\n"
            << "    </style>\n"
            << "</head>\n"
            << "<body>\n"
            << "    <h1>Internal Server Error</h1>\n"
            << "    <p>" << error_message << "</p>\n"
            << "    <p>Please try again later or contact support.</p>\n"
            << "</body>\n"
            << "</html>\n";
        return oss.str();
    }

} // namespace http_server