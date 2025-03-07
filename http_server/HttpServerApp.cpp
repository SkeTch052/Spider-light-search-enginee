#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <boost/asio.hpp>
#include <Windows.h>
#include "http_connection.h"
#include "../ini_parser.h"


void httpServer(tcp::acceptor& acceptor, tcp::socket& socket, const Config& config)
{
	acceptor.async_accept(socket,
		[&](beast::error_code ec)
		{
			if (!ec)
				std::make_shared<HttpConnection>(std::move(socket), config)->start();
			httpServer(acceptor, socket, config);
		});
}



int main(int argc, char* argv[])
{
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	try
	{
		Config config;
		try {
			config = load_config("../../config.ini");
		}
		catch (const std::exception& e) {
			std::cerr << "Failed to load config: " << e.what() << std::endl;
			return EXIT_FAILURE;
		}

		auto const address = net::ip::make_address("0.0.0.0");
		unsigned short port = static_cast<unsigned short>(config.search_port);

		net::io_context ioc{1};

		tcp::acceptor acceptor{ioc, { address, port }};
		tcp::socket socket{ioc};
		httpServer(acceptor, socket, config);

		std::cout << "Open browser and connect to http://localhost:" << port << " to see the web server operating" << std::endl;

		ioc.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
