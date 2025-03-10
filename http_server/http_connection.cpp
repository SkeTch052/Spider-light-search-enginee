﻿#include "http_connection.h"
#include "search_documents.h"
#include "../ini_parser.h"
#include "html_generator.h"

#include <sstream>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <pqxx/pqxx>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;


std::string url_decode(const std::string& encoded) {
	std::string res;
	std::istringstream iss(encoded);
	char ch;

	while (iss.get(ch)) {
		if (ch == '%') {
			int hex;
			iss >> std::hex >> hex;
			res += static_cast<char>(hex);
		}
		else if (ch == '+') {
			res += ' ';
		}
		else {
			res += ch;
		}
	}

	return res;
}

std::string convert_to_utf8(const std::string& str) {
	std::string url_decoded = url_decode(str);
	return url_decoded;
}

HttpConnection::HttpConnection(tcp::socket socket, const Config& config)
	: socket_(std::move(socket)), config_(config) {
}

void HttpConnection::start() {
	readRequest();
	checkDeadline();
}

void HttpConnection::readRequest() {
	auto self = shared_from_this();

	http::async_read(
		socket_,
		buffer_,
		request_,
		[self](beast::error_code ec,
			std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);
			if (!ec)
				self->processRequest();
		});
}

void HttpConnection::processRequest() {
	response_.version(request_.version());
	response_.keep_alive(false);

	switch (request_.method())
	{
	case http::verb::get:
		response_.result(http::status::ok);
		response_.set(http::field::server, "Beast");
		createResponseGet();
		break;
	case http::verb::post:
		response_.result(http::status::ok);
		response_.set(http::field::server, "Beast");
		createResponsePost();
		break;

	default:
		response_.result(http::status::bad_request);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body())
			<< "Invalid request-method '"
			<< std::string(request_.method_string())
			<< "'";
		break;
	}

	writeResponse();
}

std::string HttpConnection::generateStartPage() const {
	return http_server::generateStartPage();
}

// Обработка GET-запроса
void HttpConnection::createResponseGet() {
    if (request_.target().starts_with("/")) {
		response_.set(http::field::content_type, "text/html");
		beast::ostream(response_.body()) << generateStartPage();
	}
	else {
		response_.result(http::status::not_found);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body()) << "File not found\r\n";
	}
}

// Обработка POST-запроса
void HttpConnection::createResponsePost()
{
	if (request_.target() == "/") {
		std::string s = buffers_to_string(request_.body().data());

		std::cout << "POST data: " << s << std::endl;

		size_t pos = s.find('=');
		if (pos == std::string::npos) {
			response_.result(http::status::not_found);
			response_.set(http::field::content_type, "text/plain");
			beast::ostream(response_.body()) << "File not found\r\n";
			return;
		}

		std::string key = s.substr(0, pos);
		std::string value = s.substr(pos + 1);
		std::string utf8value = convert_to_utf8(value);

		if (key != "search") {
			response_.result(http::status::not_found);
			response_.set(http::field::content_type, "text/plain");
			beast::ostream(response_.body()) << "File not found\r\n";
			return;
		}

		// Проверка на пустую строку или строку из пробелов
		if (utf8value.empty() || utf8value.find_first_not_of(' ') == std::string::npos) {
			response_.set(http::field::content_type, "text/html");
			beast::ostream(response_.body()) << generateStartPage();
			return;
		}

		// Приводим строку к нижнему регистру и удаляем всё лишнее
		std::string processed_value = utf8value;
		std::transform(processed_value.begin(), processed_value.end(), processed_value.begin(),
			[](unsigned char c) { return std::tolower(c); });

		// Проверяем, содержит ли строка только латинские буквы, цифры и пробелы
		for (char c : processed_value) {
			if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == ' ')) {
				response_.result(http::status::bad_request);
				response_.set(http::field::content_type, "text/plain");
				beast::ostream(response_.body()) << "Invalid query. Please enter only latin, numbers and spaces.\r\n";
				return;
			}
		}

		std::string cleaned_value;
		std::copy_if(processed_value.begin(), processed_value.end(), std::back_inserter(cleaned_value),
			[](unsigned char c) { return std::isalnum(c) || std::isspace(c); });

		// Разделяем очищенную строку на слова
		std::vector<std::string> words;
		std::istringstream iss(cleaned_value);
		std::string word;

		while (iss >> word) {
			// Проверяем длину слова
			if (word.length() < 3 || word.length() > 32) {
				response_.result(http::status::bad_request);
				response_.set(http::field::content_type, "text/plain");
				beast::ostream(response_.body())
					<< "I can't process this word '" << word
					<< "'. Please enter words longer than 2 characters and shorter than 33 characters.\r\n";
				return;
			}
			words.push_back(word);
		}

		// Проверяем количество слов
		if (words.size() > 4) {
			response_.result(http::status::bad_request);
			response_.set(http::field::content_type, "text/plain");
			beast::ostream(response_.body()) << "Too many words. Please enter no more than 4 words.\r\n";
			return;
		}

		try {
			pqxx::connection c("host=" + config_.db_host +
							   " port=" + std::to_string(config_.db_port) +
							   " dbname=" + config_.db_name +
							   " user=" + config_.db_user +
							   " password=" + config_.db_password);
			if (!c.is_open()) {
				throw std::runtime_error("Database connection failed");
			}

			// Выполняем поиск и возвращаем результаты
			std::vector<std::pair<std::string, int>> searchResult = searchDocuments(words, c);
			response_.set(http::field::content_type, "text/html");
			beast::ostream(response_.body()) << http_server::generateSearchResults(words, searchResult);
		}
		catch (const std::exception& e) {
			// Обработка внутренней ошибки
			std::cerr << "Internal error: " << e.what() << std::endl;
			response_.result(http::status::internal_server_error);
			response_.set(http::field::content_type, "text/html");
			beast::ostream(response_.body()) << http_server::generateErrorPage("An internal error occurred: " + std::string(e.what()));
		}
	}
	else {
		response_.result(http::status::not_found);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body()) << "File not found\r\n";
	}
}

void HttpConnection::writeResponse() {
	auto self = shared_from_this();
	response_.content_length(response_.body().size());

	http::async_write(
		socket_,
		response_,
		[self](beast::error_code ec, std::size_t)
		{
			self->socket_.shutdown(tcp::socket::shutdown_send, ec);
			self->deadline_.cancel();
		});
}

// Тайм-аут для соединения
void HttpConnection::checkDeadline() {
	auto self = shared_from_this();

	deadline_.async_wait(
		[self](beast::error_code ec)
		{
			if (!ec)
			{
				self->socket_.close(ec);
			}
		});
}
