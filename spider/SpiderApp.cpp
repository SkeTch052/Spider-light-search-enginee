#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <Windows.h>
#include <functional>
#include <stdexcept>
#include <regex>
#include <chrono>

#include <pqxx/pqxx>
#include <myhtml/api.h>

#include "http_utils.h"
#include "text_processor.h"
#include "../ezParser.h"
#include "table_manager.h"
#include "db_buffer.h"
#include "extract_urls.h"
#include "parse_urls.h"

#pragma execution_character_set(utf-8)

std::mutex mtx;
std::condition_variable cv;
std::queue<std::function<void()>> tasks;
bool exitThreadPool = false;

std::unordered_set<std::string> visitedUrls;
std::mutex visitedUrlsMutex;


// Функция сохранения документов и частот с помощью буфера
void saveDocumentAndFrequency(const std::string& url, const std::string& clean, const std::map<std::string, int>& frequency, pqxx::connection& dbConn) {
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        buffer.push_back({ url, clean, frequency });
        if (buffer.size() < 10) return; // Ждём 10 записей
    }
    flushBuffer(dbConn);
}

void threadPoolWorker(pqxx::connection dbConn) {
    std::unique_lock<std::mutex> lock(mtx);
    while (!exitThreadPool || !tasks.empty()) {
        if (tasks.empty()) {
            cv.wait(lock);
        }
        else {
            auto task = tasks.front();
            tasks.pop();
            lock.unlock();
            task();
            lock.lock();
        }
    }
    // Записываем остатки буфера перед завершением потока
    flushBuffer(dbConn);
}

void prepareStatements(pqxx::connection& c) {
    pqxx::work tx(c);
    tx.exec("PREPARE insert_word (text) AS INSERT INTO words (word) VALUES ($1) ON CONFLICT DO NOTHING;");
    tx.exec("PREPARE select_word_ids (text[]) AS SELECT id, word FROM words WHERE word = ANY($1);");
    tx.exec("PREPARE insert_frequency (int, int, int) AS INSERT INTO frequency (document_id, word_id, frequency) "
            "VALUES ($1, $2, $3) ON CONFLICT (document_id, word_id) DO UPDATE SET frequency = frequency.frequency + $3;");
    tx.commit();
}

// Проверка, является ли URL ссылкой на изображение
bool isImageUrl(const std::string& url) {
    // Список расширений изображений (можно дополнить)
    const std::vector<std::string> imageExtensions = { ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp" };

    // Приводим URL к нижнему регистру для проверки
    std::string lowerUrl = url;
    for (char& c : lowerUrl) {
        c = std::tolower(c);
    }

    // Проверяем, заканчивается ли URL на одно из расширений
    for (const auto& ext : imageExtensions) {
        if (lowerUrl.length() >= ext.length() &&
            lowerUrl.substr(lowerUrl.length() - ext.length()) == ext) {
            return true;
        }
    }
    return false;
}

void parseLink(const Link& link, int depth, pqxx::connection& dbConn) {
    // Формируем базовый URL
    std::string baseUrl = (link.protocol == ProtocolType::HTTPS ? "https://" : "http://") + link.hostName;
    // Парсим начальный URL
    UrlComponents initialComponents = parseUrl(link.query, baseUrl);
    std::string fullUrl = (initialComponents.protocol == "https" ? "https://" : "http://") + initialComponents.host + initialComponents.query;

    {
        std::lock_guard<std::mutex> lock(visitedUrlsMutex);
        if (visitedUrls.count(fullUrl) > 0) {
            return;
        }
        visitedUrls.insert(fullUrl);
    }

    try {
        std::string html = getHtmlContent({ link.protocol, initialComponents.host, initialComponents.query });
        if (html.empty()) {
            std::cout << "Failed to get HTML Content or content is empty" << std::endl;
            return;
        }

        // Извлекаем ссылки только если depth > 0
        if (depth > 0) {
                        auto parseStart = std::chrono::high_resolution_clock::now();
            std::vector<std::string> urlStrings = extractUrls(html);
                        auto parseEnd = std::chrono::high_resolution_clock::now();
                        double parseDuration = std::chrono::duration_cast<std::chrono::microseconds>(parseEnd - parseStart).count() / 1000.0;

            std::vector<Link> links;
            for (const auto& url : urlStrings) {
                if (!url.empty() && url[0] != '#') {
                    UrlComponents components = parseUrl(url, fullUrl);
                    if (isImageUrl(components.query)) {
                        continue;
                    }
                    links.push_back(toLink(components));
                }
            }

            std::lock_guard<std::mutex> lock(mtx);
            for (auto& subLink : links) {
                tasks.push([subLink, depth, &dbConn]() { parseLink(subLink, depth - 1, dbConn); });
            }
            cv.notify_all();

                        std::cout << "Processed URL: " << fullUrl << std::endl;
                        std::cout << "    Parsing time: " << parseDuration << " ms" << std::endl;
        }

        std::string clean = cleanText(html);
        if (clean.empty()) {
            std::cout << "Cleaned text is empty, skipping processing" << std::endl;
            return;
        }

        std::map<std::string, int> frequency = calculateWordFrequency(clean);

                    auto dbStart = std::chrono::high_resolution_clock::now();
        saveDocumentAndFrequency(fullUrl, clean, frequency, dbConn);
                    auto dbEnd = std::chrono::high_resolution_clock::now();
                    double dbDuration = std::chrono::duration_cast<std::chrono::microseconds>(dbEnd - dbStart).count() / 1000.0;
                    std::cout << "Processed URL (max depth): " << fullUrl << std::endl << "    DB write time: " << dbDuration << " ms" << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "Error in parseLink: " << e.what() << std::endl;
    }
}



int main() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    setvbuf(stdout, nullptr, _IOFBF, 1000);

    Config config;
    try {
        config = load_config("../../config.ini");
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load config: " << e.what() << std::endl;
        return 1;
    }

    // Подключение к БД для основного потока
    pqxx::connection mainDbConn("host=" + config.db_host +
                                " port=" + std::to_string(config.db_port) +
                                " dbname=" + config.db_name +
                                " user=" + config.db_user +
                                " password=" + config.db_password);
    if (!mainDbConn.is_open()) {
        std::cerr << "Failed to connect to database\n";
        return 1;
    }

    create_tables(mainDbConn);
    prepareStatements(mainDbConn);

    try {
        int numThreads = std::thread::hardware_concurrency();
        std::vector<std::thread> threadPool;
        std::vector<pqxx::connection> threadConnections;

        for (int i = 0; i < numThreads; ++i) {
            threadConnections.emplace_back("host=" + config.db_host +
                                           " port=" + std::to_string(config.db_port) +
                                           " dbname=" + config.db_name +
                                           " user=" + config.db_user +
                                           " password=" + config.db_password);
            if (!threadConnections.back().is_open()) {
                std::cerr << "Failed to connect thread " << i << " to database\n";
                return 1;
            }
            prepareStatements(threadConnections.back());
            threadPool.emplace_back(threadPoolWorker, std::move(threadConnections[i]));
        }

        Link link{ ProtocolType::HTTPS, "en.wikipedia.org", "/wiki/Main_Page" };
        {
            std::lock_guard<std::mutex> lock(mtx);
            tasks.push([link, &mainDbConn, depth = config.depth]() { parseLink(link, depth, mainDbConn); });
            cv.notify_one();
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));

        {
            std::lock_guard<std::mutex> lock(mtx);
            exitThreadPool = true;
            cv.notify_all();
        }

        for (auto& t : threadPool) {
            t.join();
        }

        flushBuffer(mainDbConn);
    }
    catch (const std::exception& e) {
        std::cout << "Main error: " << e.what() << std::endl;
    }
    return 0;
}
