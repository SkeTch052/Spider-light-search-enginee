#pragma once

#include <vector>
#include <mutex>
#include <string>
#include <map>
#include <pqxx/pqxx>

struct DocumentData {
    std::string url;
    std::string cleanText;
    std::map<std::string, int> frequency;
};

extern std::vector<DocumentData> buffer;
extern std::mutex bufferMutex;

void flushBuffer(pqxx::connection& dbConn);