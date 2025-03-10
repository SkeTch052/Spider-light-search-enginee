#include "search_documents.h"

// Функция поиска документов по словам
std::vector<std::pair<std::string, int>> searchDocuments(const std::vector<std::string>& words, pqxx::connection& c) {
    std::vector<std::pair<std::string, int>> results;
    pqxx::work txn(c);

    // Сначала получаем word_id для каждого слова
    std::vector<int> word_ids;
    for (const auto& word : words) {

        std::string query = "SELECT id FROM words WHERE word = " + txn.quote(word) + ";";
        pqxx::result res = txn.exec(query);

        if (res.empty()) {
            // Если хотя бы одно слово не найдено, возвращаем пустой результат
            std::cout << "Word not found: " << word << std::endl;
            return results;
        }
        word_ids.push_back(res[0][0].as<int>());
    }

    // Формируем строку со списком word_ids отдельно для вставки в запрос
    std::ostringstream word_ids_stream;
    for (size_t i = 0; i < word_ids.size(); ++i) {
      word_ids_stream << word_ids[i];
      if (i < word_ids.size() - 1) {
        word_ids_stream << ", ";
      }
    }
    std::string word_ids_str = word_ids_stream.str();

    // Формируем SQL-запрос для поиска документов, содержащих все слова
    std::string query = R"(
        SELECT d.id, d.url, SUM(f.frequency) as total_frequency
        FROM documents d
        JOIN frequency f ON d.id = f.document_id
        WHERE f.word_id IN ()" +
                        word_ids_str + R"()
        GROUP BY d.id, d.url
        HAVING COUNT(DISTINCT f.word_id) = )" +
                        std::to_string(word_ids.size()) + R"(
        ORDER BY total_frequency DESC
        LIMIT 10;)";

    // Выполняем запрос и собираем результаты
    pqxx::result res = txn.exec(query);
    for (const auto& row : res) {
        std::string url = row[1].as<std::string>();
        int total_frequency = row[2].as<int>();
        results.emplace_back(url, total_frequency);
    }

    txn.commit();
    return results;
}