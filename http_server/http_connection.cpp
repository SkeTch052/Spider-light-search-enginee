#include "http_connection.h"
#include "search_documents.h"
#include "../ezParser.h"

#include <sstream>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <iostream>
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
		else if (ch == '+') { // Заменяем '+' на пробел
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

HttpConnection::HttpConnection(tcp::socket socket)
	: socket_(std::move(socket)) {}

void HttpConnection::start()
{
	readRequest();
	checkDeadline();
}


void HttpConnection::readRequest()
{
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

void HttpConnection::processRequest()
{
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
	std::ostringstream oss;
	oss << "<html>\n"
		<< "<head>\n"
		<< "    <meta charset=\"UTF-8\">\n"
		<< "    <title>Search Engine</title>\n"
		<< "    <style>\n"
		<< "        body {\n"
		<< "            display: flex;\n"
		<< "            justify-content: center;\n"
		<< "            align-items: center;\n"
		<< "            height: 100vh;\n"
		<< "            margin-top: -10vh;\n"
		<< "            flex-direction: column;\n"
		<< "        }\n"
		<< "        h1 {\n"
		<< "            margin-bottom: 20px;\n"
		<< "        }\n"
		<< "        form {\n"
		<< "            text-align: center;\n"
		<< "        }\n"
		<< "        input[type=\"text\"] {\n"
		<< "            padding: 10px;\n"
		<< "            font-size: 16px;\n"
		<< "            width: 300px;\n"
		<< "            height: 30px;\n"
		<< "            margin-bottom: 10px;\n"
		<< "            box-sizing: border-box;\n"
		<< "        }\n"
		<< "        input[type=\"submit\"] {\n"
		<< "            font-size: 16px;\n"
		<< "            height: 30px;\n"
		<< "            cursor: pointer;\n"
		<< "            box-sizing: border-box;\n"
		<< "        }\n"
		<< "    </style>\n"
		<< "</head>\n"
		<< "<body>\n"
		<< "    <h1>Search Engine</h1>\n"
		<< "    <p>Welcome!</p>\n"
		<< "    <form action=\"/\" method=\"post\">\n"
		<< "        <input type=\"text\" id=\"search\" name=\"search\"><br>\n"
		<< "        <input type=\"submit\" value=\"Search\">\n"
		<< "    </form>\n"
		<< "</body>\n"
		<< "</html>\n";
	return oss.str();
}

void HttpConnection::createResponseGet()
{
	if (request_.target() == "/")
	{
		response_.set(http::field::content_type, "text/html");
		beast::ostream(response_.body()) << generateStartPage();
	}
	else
	{
		response_.result(http::status::not_found);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body()) << "File not found\r\n";
	}
}

void HttpConnection::createResponsePost()
{
	if (request_.target() == "/")
	{
		std::string s = buffers_to_string(request_.body().data());

		std::cout << "POST data: " << s << std::endl;

		size_t pos = s.find('=');
		if (pos == std::string::npos)
		{
			response_.result(http::status::not_found);
			response_.set(http::field::content_type, "text/plain");
			beast::ostream(response_.body()) << "File not found\r\n";
			return;
		}

		std::string key = s.substr(0, pos);
		std::string value = s.substr(pos + 1);

		std::string utf8value = convert_to_utf8(value);

		if (key != "search")
		{
			response_.result(http::status::not_found);
			response_.set(http::field::content_type, "text/plain");
			beast::ostream(response_.body()) << "File not found\r\n";
			return;
		}

		// Проверка на пустую строку или строку из пробелов
		if (utf8value.empty() || utf8value.find_first_not_of(' ') == std::string::npos)
		{
			response_.set(http::field::content_type, "text/html");
			beast::ostream(response_.body()) << generateStartPage();
			return;
		}

		// Разделяем строку на слова
		std::vector<std::string> words;
		std::istringstream iss(utf8value);
		std::string word;

		while (iss >> word) {
			// Проверяем длину слова
			if (word.length() < 3 || word.length() > 32) { // Слова от 3 до 32 символов
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

		Config config;
		try {
			config = load_config("../../config.ini");
		}
		catch (const std::exception& e) {
			std::cerr << "Failed to load config: " << e.what() << std::endl;
		}

		//Создание подключения к базе данных
		pqxx::connection c("host=" + config.db_host + " port=" + std::to_string(config.db_port) + " dbname=" + config.db_name + " user=" + config.db_user + " password=" + config.db_password);
		if (!c.is_open()) {
			std::cerr << "Failed to connect to database" << std::endl;
		}

		// Выполняем поиск
		std::vector<std::pair<std::string, int>> searchResult = searchDocuments(words, c);

		// Формирование HTML ответа
		response_.set(http::field::content_type, "text/html");
		beast::ostream(response_.body())
			<< "<html>\n"
			<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
			<< "<body>\n"
			<< "<h1>Search Engine</h1>\n"
			<< "<p>Response:<p>\n"
			<< "<ul>\n";

		if (searchResult.empty()) {
			beast::ostream(response_.body()) << "<li>No results for this query.</li>";
		}
		else {
			for (const auto& result : searchResult) {
				beast::ostream(response_.body())
					<< "<li><a href=\"" << result.first << "\">" << result.first << "</a> (Relevance: " << result.second << ")</li>";
			}
		}

		beast::ostream(response_.body())
			<< "</ul>\n"
			<< "</body>\n"
			<< "</html>\n";
	}
	else
	{
		response_.result(http::status::not_found);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body()) << "File not found\r\n";
	}
}

void HttpConnection::writeResponse()
{
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

void HttpConnection::checkDeadline()
{
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
