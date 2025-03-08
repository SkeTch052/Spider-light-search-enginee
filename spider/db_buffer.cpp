#include "db_buffer.h"

#include <iostream>

std::vector<DocumentData> buffer; // Буфер для хранения данных документов
std::mutex bufferMutex;           // Мьютекс для синхронизации доступа к буферу

// Функция для записи буфера в БД с пакетной вставкой
void flushBuffer(pqxx::connection& dbConn) {
    std::vector<DocumentData> localBuffer;

    // Переносим данные из глобального буфера в локальный
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        localBuffer.swap(buffer);
    }
    if (localBuffer.empty()) return;

    try {
        pqxx::work txn(dbConn);

        // Перебираем все документы в локальном буфере
        for (const auto& doc : localBuffer) {
            pqxx::result checkResult = txn.exec("SELECT id FROM documents WHERE url = " + txn.quote(doc.url) + ";");
            int docId;

            // Если документ не существует, вставляем его
            if (checkResult.empty()) {
                std::string query = "INSERT INTO documents (url, content) VALUES (" +
                    txn.quote(doc.url) + ", " + txn.quote(doc.cleanText) + ") RETURNING id;";
                pqxx::result result = txn.exec(query);

                if (result.empty()) {
                    throw std::runtime_error("Failed to insert document into the database.");
                }

                docId = result[0][0].as<int>();
            }
            else {
                // Если документ существует, обновляем содержимое
                docId = checkResult[0][0].as<int>();
                txn.exec("UPDATE documents SET content = " + txn.quote(doc.cleanText) + " WHERE id = " + txn.quote(docId) + ";");
            }

            // Если есть данные о частоте слов, то обрабатываем их
            if (!doc.frequency.empty()) {
                std::string wordInsertQuery = "INSERT INTO words (word) VALUES ";
                std::vector<std::string> words;

                for (const auto& [word, freq] : doc.frequency) {
                    words.push_back(word);
                    wordInsertQuery += "(" + txn.quote(word) + "),";
                }

                wordInsertQuery.pop_back(); // Убираем последнюю запятую
                wordInsertQuery += " ON CONFLICT DO NOTHING;";
                txn.exec(wordInsertQuery);

                // Получаем ID вставленных или существующих слов
                pqxx::result wordsRes = txn.exec_prepared("select_word_ids", words);
                std::map<std::string, int> wordIdMap;
                for (const auto& row : wordsRes) {
                    wordIdMap[row["word"].as<std::string>()] = row["id"].as<int>();
                }

                // Формируем запрос для пакетной вставки частот в таблицу frequency
                std::string freqInsertQuery = "INSERT INTO frequency (document_id, word_id, frequency) VALUES ";
                for (const auto& [word, freq] : doc.frequency) {
                    auto it = wordIdMap.find(word);
                    if (it != wordIdMap.end()) {
                        // Добавляем запись о частоте слова для документа
                        freqInsertQuery += "(" + std::to_string(docId) + "," +
                            std::to_string(it->second) + "," +
                            std::to_string(freq) + "),";
                    }
                }
                freqInsertQuery.pop_back(); // Убираем последнюю запятую
                //Если частота существует, обновляем содержимое
                freqInsertQuery += " ON CONFLICT (document_id, word_id) DO UPDATE SET frequency = frequency.frequency + EXCLUDED.frequency;";

                txn.exec(freqInsertQuery);
            }
        }

        txn.commit();
    }
    catch (const std::exception& e) {
        std::cerr << "Error in flushBuffer: " << e.what() << std::endl;
    }
}