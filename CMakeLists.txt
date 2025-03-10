cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0144 NEW)

# Единый проект для обоих приложений
project(SearchEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PostgreSQL_ADDITIONAL_VERSIONS "17")

# HttpServerApp
add_executable(HttpServerApp
    http_server/HttpServerApp.cpp
    http_server/html_generator.h
    http_server/html_generator.cpp
    http_server/http_connection.h
    http_server/http_connection.cpp
    http_server/search_documents.h
    http_server/search_documents.cpp
    config.ini
    ini_parser.h
    ini_parser.cpp
)

# SpiderApp
add_executable(SpiderApp
    spider/SpiderApp.cpp
    spider/db_buffer.cpp
    spider/db_buffer.h
    spider/extract_urls.h
    spider/http_utils.cpp
    spider/http_utils.h
    spider/link.h
    spider/parse_urls.h
    spider/table_manager.cpp
    spider/table_manager.h
    spider/text_processor.h
    config.ini
    ini_parser.h
    ini_parser.cpp
)

# Установка выходного пути для статических библиотек
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Статическая библиотека inih
add_library(inih STATIC
    inih-r58/ini.c
    inih-r58/ini.h
    inih-r58/cpp/INIReader.cpp
    inih-r58/cpp/INIReader.h
)
target_include_directories(inih PUBLIC 
    "${CMAKE_SOURCE_DIR}/inih-r58"
    "${CMAKE_SOURCE_DIR}/inih-r58/cpp"
)

# Подключение libpqxx
set(LIBPQXX_DIR "C:/Program Files/pqxx/libpqxx-7.10.0" CACHE PATH "Path to libpqxx")
if(NOT EXISTS "${LIBPQXX_DIR}")
    message(FATAL_ERROR "libpqxx directory not found: ${LIBPQXX_DIR}")
endif()
add_subdirectory("${LIBPQXX_DIR}" libpqxx-build)

# Настройка MyHTML
set(MYHTML_INSTALL_DIR "C:/Program Files/MyHTML/myhtml-4.0.5/build/install" CACHE PATH "Path to MyHTML install directory")
set(MYHTML_INCLUDE_DIR "${MYHTML_INSTALL_DIR}/include")
set(MYHTML_LIB_DIR "${MYHTML_INSTALL_DIR}/lib")
add_library(myhtml STATIC IMPORTED)
set_target_properties(myhtml PROPERTIES 
    IMPORTED_LOCATION "${MYHTML_LIB_DIR}/myhtml_static.lib"
)

# Настройка Boost
set(BOOST_ROOT "C:/Program Files/Boost/boost-1.87.0" CACHE PATH "Path to Boost")
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.87 REQUIRED COMPONENTS system)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found! Please check BOOST_ROOT.")
endif()

# OpenSSL
find_package(OpenSSL REQUIRED)

# Инклуды для HttpServerApp
target_include_directories(HttpServerApp PRIVATE 
    ${Boost_INCLUDE_DIRS}
)

# Инклуды для SpiderApp
target_include_directories(SpiderApp PRIVATE 
    ${Boost_INCLUDE_DIRS}
    ${MYHTML_INCLUDE_DIR}
)

# Линковка библиотек для HttpServerApp
target_link_libraries(HttpServerApp PRIVATE 
    inih
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    pqxx
)

# Линковка библиотек для SpiderApp
target_link_libraries(SpiderApp PRIVATE 
    inih
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    pqxx
    myhtml
)