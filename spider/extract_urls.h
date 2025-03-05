#pragma once

#include <vector>
#include <string>
#include <myhtml/api.h>

// Функция для извлечения URL из HTML с помощью библиотеки myhtml
inline std::vector<std::string> extractUrls(const std::string& html) {

    std::vector<std::string> urls;

    // Инициализация объекта myhtml
    myhtml_t* myhtml = myhtml_create();
    if (!myhtml) return urls;

    // Инициализация myhtml с настройками по умолчанию
    myhtml_init(myhtml, MyHTML_OPTIONS_DEFAULT, 1, 0);

    // Создание дерева разбора HTML
    myhtml_tree_t* tree = myhtml_tree_create();
    if (!tree) {
        myhtml_destroy(myhtml);
        return urls;
    }

    // Инициализация дерева для парсинга
    myhtml_tree_init(tree, myhtml);

    // Парсинг HTML-документа с указанием кодировки UTF-8
    mystatus_t status = myhtml_parse(tree, MyENCODING_UTF_8, html.c_str(), html.length());
    if (status != MyHTML_STATUS_OK) {
        myhtml_tree_destroy(tree);
        myhtml_destroy(myhtml);
        return urls;
    }

    // Получение коллекции всех тегов <a> в документе
    myhtml_collection_t* collection = myhtml_get_nodes_by_name(tree, NULL, "a", 1, NULL);
    if (collection && collection->list) {
        for (size_t i = 0; i < collection->length; ++i) {
            myhtml_tree_node_t* node = collection->list[i]; // Текущий узел <a>

            // Получение первого атрибута узла
            myhtml_tree_attr_t* attr = myhtml_node_attribute_first(node);
            while (attr) {
                // Получение ключа атрибута (имени)
                const char* key = myhtml_attribute_key(attr, NULL);
                if (key && strcmp(key, "href") == 0) { // Проверяем, является ли атрибут href
                    // Получение значения атрибута href
                    const char* value = myhtml_attribute_value(attr, NULL);
                    if (value) {
                        urls.push_back(std::string(value));
                    }
                    break; // Выходим из цикла, если href найден
                }
                attr = myhtml_attribute_next(attr); // Переход к следующему атрибуту
            }
        }
    }

    // Очистка ресурсов
    if (collection) myhtml_collection_destroy(collection);
    myhtml_tree_destroy(tree);
    myhtml_destroy(myhtml);

    return urls;
}
