#pragma once
#include <regex>
#include <boost/locale.hpp>
#include <map>
#include <sstream>
#include <string>
#include <stdexcept>
#include <iostream>

// Функция удаляет HTML-теги, пунктуацию и переводит текст в нижний регистр
std::string cleanText(const std::string& html) {
    try {
        boost::locale::generator gen;
        std::locale loc = gen("");
        std::locale::global(loc);
        std::cout.imbue(loc);

        if (html.empty()) {
            return "";
        }

        // Удаляем HTML-теги, но сохраняем пробелы
        std::string clean = std::regex_replace(html, std::regex("<.*?>"), " ");

        // Удаляем пунктуацию, но сохраняем пробелы
        clean = std::regex_replace(clean, std::regex(R"([^\w\s])"), " ");

        // Переводим в нижний регистр
        clean = boost::locale::to_lower(clean);

        // Удаляем лишние пробелы
        clean = std::regex_replace(clean, std::regex(R"(\s+)"), " ");
        clean = std::regex_replace(clean, std::regex(R"(^\s+|\s+$)"), "");

        return clean;
    }
    catch (const std::exception& e) {
        std::cerr << "Error in cleanText: " << e.what() << std::endl;
        return "";
    }
}

// Функция для подсчета частоты слов
std::map<std::string, int> calculateWordFrequency(const std::string& text) {
    std::map<std::string, int> frequency;
    std::istringstream stream(text);

    std::string word;
    while (stream >> word) {
        if (word.length() > 3 && word.length() < 32) {
            frequency[word]++;
        }
    }
    return frequency;
}