#include "table_manager.h"
#include <iostream>

void create_tables(pqxx::connection& c) {
    try {
        pqxx::work tx(c);
        tx.exec("CREATE TABLE IF NOT EXISTS documents ("
                "id SERIAL PRIMARY KEY,"
                "url TEXT NOT NULL,"
                "content TEXT,"
                "date_created TIMESTAMP DEFAULT CURRENT_TIMESTAMP);");

        tx.exec("CREATE TABLE IF NOT EXISTS words ("
                "id SERIAL PRIMARY KEY,"
                "word TEXT NOT NULL UNIQUE);");

        tx.exec("CREATE TABLE IF NOT EXISTS frequency ("
                "document_id INT REFERENCES documents(id),"
                "word_id INT REFERENCES words(id),"
                "frequency INT,"
                "PRIMARY KEY (document_id, word_id));");

        tx.commit();
        std::cout << "Tables created successfully!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error while creating tables: " << e.what() << std::endl;
        throw;
    }
}
