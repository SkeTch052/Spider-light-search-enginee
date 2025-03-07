cmake_minimum_required(VERSION 3.20)

cmake_policy(SET CMP0144 NEW)

project(HttpServerApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PostgreSQL_ADDITIONAL_VERSIONS "17")

add_executable(HttpServerApp
    HttpServerApp.cpp
    html_generator.h
    html_generator.cpp
    http_connection.h
    http_connection.cpp
    search_documents.h
    search_documents.cpp
    ../config.ini
    ../ini_parser.h
    ../ini_parser.cpp
)

# Статическая библиотека inih
add_library(inih STATIC
    ../inih-r58/ini.c
    ../inih-r58/ini.h
    ../inih-r58/cpp/INIReader.cpp
    ../inih-r58/cpp/INIReader.h
)

# Инклуды для inih
target_include_directories(inih PUBLIC 
    "${CMAKE_SOURCE_DIR}/../inih-r58"
    "${CMAKE_SOURCE_DIR}/../inih-r58/cpp"
)

# Подключение libpqxx
add_subdirectory("C:/Program Files/pqxx/libpqxx-7.10.0" libpqxx-build)

# Настройка Boost
set(BOOST_ROOT "C:/Program Files/Boost/boost-1.87.0")
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

# system нужен для asio/beast
find_package(Boost 1.87 REQUIRED COMPONENTS system)  

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found! Please check BOOST_ROOT.")
endif()

# OpenSSL (должен быть в PATH или указан вручную)
find_package(OpenSSL REQUIRED)

# Инклуды для HttpServerApp
target_include_directories(HttpServerApp PRIVATE 
    ${Boost_INCLUDE_DIRS}
)

# Линковка библиотек
target_link_libraries(HttpServerApp PRIVATE 
    inih
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    pqxx
)
