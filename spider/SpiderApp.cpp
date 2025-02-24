#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <pqxx/pqxx>
#include <Windows.h>
#include <functional>
#include <stdexcept>

#include "http_utils.h"
#include "text_processor.h"
#include "../ezParser.h"
#include "table_manager.h"

#pragma execution_character_set(utf-8)


std::mutex mtx;
std::condition_variable cv;
std::queue<std::function<void()>> tasks;
bool exitThreadPool = false;


void threadPoolWorker() {
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
}

void saveDocumentAndFrequency(const std::string& url, const std::string& cleanText, const std::map<std::string, int>& frequency, pqxx::connection& c) {
	try {
		pqxx::work tx(c);

		// Проверяем, существует ли уже документ с таким URL
		pqxx::result checkResult = tx.exec("SELECT id FROM documents WHERE url = " + tx.quote(url) + ";");

		int docId;
		if (checkResult.empty()) {
			// Записываем сам документ, если его еще нет
			std::string query = "INSERT INTO documents (url, content) VALUES (" +
				tx.quote(url) + ", " +
				tx.quote(cleanText) + ") RETURNING id;";
			pqxx::result result = tx.exec(query);

			if (result.empty()) {
				throw std::runtime_error("Failed to insert document into the database.");
			}

			docId = result[0][0].as<int>();
		}
		else {
			// Используем существующий id
			docId = checkResult[0][0].as<int>();
			tx.exec("UPDATE documents SET content = " + tx.quote(cleanText) + " WHERE id = " + tx.quote(docId) + ";");
		}

		// Записываем слова и частоты
		for (const auto& [word, freq] : frequency) {
			// Вставляем слово или ничего, если уже существует
			tx.exec("INSERT INTO words (word) VALUES (" + tx.quote(word) + ") ON CONFLICT DO NOTHING;");

			// Получаем id слова
			pqxx::result wordRes = tx.exec("SELECT id FROM words WHERE word = " + tx.quote(word) + ";");

			if (wordRes.empty()) {
				throw std::runtime_error("Failed to retrieve word ID from the database.");
			}

			int wordId = wordRes[0][0].as<int>();

			// Проверяем существует ли связь и обновляем частоту или добавляем новую запись
			tx.exec("INSERT INTO frequency (document_id, word_id, frequency) VALUES (" +
				tx.quote(docId) + ", " + tx.quote(wordId) + ", " + tx.quote(freq) + ") " +
				"ON CONFLICT (document_id, word_id) DO UPDATE SET frequency = frequency.frequency + " + tx.quote(freq) + ";");
		}

		tx.commit();
	}
	catch (const std::exception& e) {
		std::cerr << "Error saving document and frequency: " << e.what() << std::endl;
	}
}

void parseLink(const Link& link, int depth, pqxx::connection& dbConn)
{
	try {

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		std::string html = getHtmlContent(link);

		if (html.empty()) {
			std::cout << "Failed to get HTML Content or content is empty" << std::endl;
			return;
		}

		// TODO: Parse HTML code here on your own
		
		std::string clean = cleanText(html);// Очистка текста

		if (clean.empty()) {
			std::cout << "Cleaned text is empty, skipping processing" << std::endl;
			return;
		}

		// Подсчет частоты слов
		std::map<std::string, int> frequency = calculateWordFrequency(clean);

		// Сохранение в базу данных
		saveDocumentAndFrequency(link.hostName + link.query, clean, frequency, dbConn);

		// TODO: Collect more links from HTML code and add them to the parser like that:

		//std::vector<Link> links = {
		//{ProtocolType::HTTPS, "en.wikipedia.org", "/wiki/Wikipedia"},
		//	{ProtocolType::HTTPS, "wikimediafoundation.org", "/"},
		//};

		// Сбор ссылок из HTML
		std::regex link_regex("<a\\s+href=\"([^\"]+)\"", std::regex_constants::icase);
		std::smatch matches;
		std::string::const_iterator searchStart(html.cbegin());
		std::vector<Link> links;

		while (std::regex_search(searchStart, html.cend(), matches, link_regex)) {
			std::string url = matches[1].str();

			if (!url.empty() && url[0] != '#') {
				if (url.find("http") != 0) {
					url = link.hostName + url;
				}

				links.push_back({ link.protocol, link.hostName, url });
			}

			searchStart = matches.suffix().first;
		}

		if (depth > 0) {
			std::lock_guard<std::mutex> lock(mtx);

			size_t count = links.size();
			size_t index = 0;
			for (auto& subLink : links)
			{
				tasks.push([subLink, depth, &dbConn]() { parseLink(subLink, depth - 1, dbConn); });
			}
			cv.notify_one();
		}
	}
	catch (const std::exception& e)
	{
		std::cout << "Error in parseLink: " << e.what() << std::endl;
	}

}



int main()
{
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

	pqxx::connection dbConn("host=" + config.db_host + " port=" + std::to_string(config.db_port) + " dbname=" + config.db_name + " user=" + config.db_user + " password=" + config.db_password);
	create_tables(dbConn);

	if (!dbConn.is_open()) {
		std::cerr << "Failed to connect to database\n";
		return 1;
	}

	try {
		int numThreads = std::thread::hardware_concurrency();
		std::vector<std::thread> threadPool;

		for (int i = 0; i < numThreads; ++i) {
			threadPool.emplace_back(threadPoolWorker);
		}

		Link link{ ProtocolType::HTTPS, "en.wikipedia.org", "/wiki/Main_Page" };

		{
			std::lock_guard<std::mutex> lock(mtx);
			tasks.push([link, &dbConn]() { parseLink(link, 1, dbConn); });
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
	}
	catch (const std::exception& e)
	{
		std::cout << "Main error: " << e.what() << std::endl;
	}
	return 0;
}
