#include "db_buffer.h"

#include <iostream>

std::vector<DocumentData> buffer; // Буфер для хранения данных документов
std::mutex bufferMutex;           // Мьютекс для синхронизации доступа к буферу

// Функция записи накопленных данных в БД
void flushBuffer(pqxx::connection& dbConn) {
    std::vector<DocumentData> localBuffer;

    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        localBuffer.swap(buffer); // Переносим данные в локальный буфер
    }
    if (localBuffer.empty()) return;

    try {
        pqxx::work txn(dbConn);

        for (const auto& doc : localBuffer) {
            // Проверяем, существует ли документ
            pqxx::result checkResult = txn.exec("SELECT id FROM documents WHERE url = " + txn.quote(doc.url) + ";");
            int docId;

            if (checkResult.empty()) { // Новый документ
                std::string query = "INSERT INTO documents (url, content) VALUES (" +
                    txn.quote(doc.url) + ", " + txn.quote(doc.cleanText) + ") RETURNING id;";
                pqxx::result result = txn.exec(query);

                if (result.empty()) {
                    throw std::runtime_error("Failed to insert document into the database.");
                }

                docId = result[0][0].as<int>();
            }
            else { // Обновляем существующий документ
                docId = checkResult[0][0].as<int>();
                txn.exec("UPDATE documents SET content = " + txn.quote(doc.cleanText) + " WHERE id = " + txn.quote(docId) + ";");
            }

            if (!doc.frequency.empty()) { // Обрабатываем частоту слов
                std::string wordInsertQuery = "INSERT INTO words (word) VALUES ";
                std::vector<std::string> words;

                for (const auto& [word, freq] : doc.frequency) {
                    words.push_back(word);
                    wordInsertQuery += "(" + txn.quote(word) + "),";
                }

                wordInsertQuery.pop_back();
                wordInsertQuery += " ON CONFLICT DO NOTHING;";
                txn.exec(wordInsertQuery);

                // Получаем ID слов из базы
                pqxx::result wordsRes = txn.exec_prepared("select_word_ids", words);
                std::map<std::string, int> wordIdMap;
                for (const auto& row : wordsRes) {
                    wordIdMap[row["word"].as<std::string>()] = row["id"].as<int>();
                }

                // Вставляем частоты слов для документа
                std::string freqInsertQuery = "INSERT INTO frequency (document_id, word_id, frequency) VALUES ";
                for (const auto& [word, freq] : doc.frequency) {
                    auto it = wordIdMap.find(word);

                    if (it != wordIdMap.end()) {
                        freqInsertQuery += "(" + std::to_string(docId) + "," +
                            std::to_string(it->second) + "," +
                            std::to_string(freq) + "),";
                    }
                }
                freqInsertQuery.pop_back();
                // Обновляем существующую частоту
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