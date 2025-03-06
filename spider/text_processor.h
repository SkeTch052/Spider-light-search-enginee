#pragma once
#include <string>
#include <map>
#include <sstream>
#include <cctype>
#include <myhtml/api.h>

// Рекурсивное извлечение текста из узлов
inline void extractTextFromNode(myhtml_tree_node_t* node, std::stringstream& result) {
    while (node) {
        const char* text = (const char*)myhtml_node_text(node, nullptr);
        if (text) {
            result << text << " ";
        }
        myhtml_tree_node_t* child = myhtml_node_child(node);
        if (child) {
            extractTextFromNode(child, result);
        }
        node = myhtml_node_next(node);
    }
}

// Извлечение текста из HTML с помощью myhtml
inline std::string extractTextFromHtml(const std::string& html) {
    if (html.empty()) {
        return "";
    }

    // Инициализация myhtml
    myhtml_t* myhtml = myhtml_create();
    if (!myhtml) return "";

    myhtml_init(myhtml, MyHTML_OPTIONS_DEFAULT, 1, 0);

    myhtml_tree_t* tree = myhtml_tree_create();
    if (!tree) {
        myhtml_destroy(myhtml);
        return "";
    }

    myhtml_tree_init(tree, myhtml);

    // Парсинг HTML с кодировкой UTF-8
    mystatus_t status = myhtml_parse(tree, MyENCODING_UTF_8, html.c_str(), html.length());
    if (status != MyHTML_STATUS_OK) {
        myhtml_tree_destroy(tree);
        myhtml_destroy(myhtml);
        return "";
    }

    // Извлечение текста
    std::stringstream result;
    myhtml_tree_node_t* node = myhtml_tree_get_node_body(tree);
    if (node) {
        extractTextFromNode(node, result);
    }

    myhtml_tree_destroy(tree);
    myhtml_destroy(myhtml);

    return result.str();
}

// Очистка текста с фильтрацией слов по длине (3-32 символа)
inline std::string cleanText(const std::string& html) {
    try {
        // Извлекаем текст из HTML
        std::string text = extractTextFromHtml(html);
        if (text.empty()) {
            return "";
        }

        // Обрабатываем текст посимвольно
        std::string clean;
        clean.reserve(text.size()); // Предварительно выделяем память
        bool lastWasSpace = true;

        for (char c : text) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                clean += std::tolower(static_cast<unsigned char>(c));
                lastWasSpace = false;
            }
            else if (std::isspace(static_cast<unsigned char>(c)) && !lastWasSpace) {
                clean += ' ';
                lastWasSpace = true;
            }
        }

        // Удаляем конечные пробелы
        if (!clean.empty() && clean.back() == ' ') {
            clean.pop_back();
        }

        // Фильтрация слов по длине (3-32 символа)
        std::stringstream filtered;
        std::istringstream iss(clean);
        std::string word;
        bool firstWord = true;

        while (iss >> word) {
            if (word.length() >= 3 && word.length() <= 32) {
                if (!firstWord) {
                    filtered << " ";
                }
                filtered << word;
                firstWord = false;
            }
        }

        return filtered.str();
    }
    catch (const std::exception& e) {
        std::cerr << "Error in cleanText: " << e.what() << std::endl;
        return "";
    }
}

// Подсчёт частоты слов без фильтрации по длине
inline std::map<std::string, int> calculateWordFrequency(const std::string& text) {
    std::map<std::string, int> frequency;
    std::istringstream stream(text);
    std::string word;

    while (stream >> word) {
        frequency[word]++;
    }
    return frequency;
}
