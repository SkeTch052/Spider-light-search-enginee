#pragma once
#include <string>
#include "link.h"

// Структура для представления разобранного URL
struct UrlComponents {
    std::string protocol;
    std::string host;
    std::string query;
};

// Функция для разбора URL на составляющие
inline UrlComponents parseUrl(const std::string& url, const std::string& baseUrl = "") {
    UrlComponents components;

    // Если URL начинается с http:// или https://, парсим его полностью
    if (url.find("http://") == 0 || url.find("https://") == 0) {
        size_t protoEnd = url.find("://");
        components.protocol = url.substr(0, protoEnd);
        size_t hostStart = protoEnd + 3;
        size_t hostEnd = url.find('/', hostStart);
        if (hostEnd == std::string::npos) hostEnd = url.length();
        components.host = url.substr(hostStart, hostEnd - hostStart);
        components.query = (hostEnd == url.length()) ? "/" : url.substr(hostEnd);
    }
    // Если URL относительный
    else {
        if (!baseUrl.empty()) {
            // Парсим базовый URL, чтобы извлечь чистый хост
            size_t protoEnd = baseUrl.find("://");
            std::string baseProtocol = protoEnd != std::string::npos ? baseUrl.substr(0, protoEnd) : "http";
            size_t hostStart = protoEnd != std::string::npos ? protoEnd + 3 : 0;
            size_t hostEnd = baseUrl.find('/', hostStart);
            if (hostEnd == std::string::npos) hostEnd = baseUrl.length();
            std::string baseHost = baseUrl.substr(hostStart, hostEnd - hostStart);

            if (url.empty() || url[0] == '/') {
                // Абсолютный путь относительно базового URL
                components.protocol = baseProtocol;
                components.host = baseHost;
                components.query = url.empty() ? "/" : url;
            }
            else {
                // Относительный путь
                size_t lastSlash = baseUrl.find_last_of('/');
                std::string basePath = (lastSlash == std::string::npos || lastSlash < hostEnd) ? baseUrl.substr(0, hostEnd) : baseUrl.substr(0, lastSlash);
                components.protocol = baseProtocol;
                components.host = baseHost;
                components.query = basePath + "/" + url;
            }
        }
        else {
            // Если нет базового URL, то считаем некорректным
            components.query = url;
        }
    }

    return components;
}

// Преобразование UrlComponents в Link
inline Link toLink(const UrlComponents& components) {
    return {
        components.protocol == "https" ? ProtocolType::HTTPS : ProtocolType::HTTP,
        components.host,
        components.query
    };
}