cmake_minimum_required(VERSION 3.20)

cmake_policy(SET CMP0144 NEW)

project(SpiderApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PostgreSQL_ADDITIONAL_VERSIONS "17")

add_executable(SpiderApp
    SpiderApp.cpp
    db_buffer.cpp
    db_buffer.h
    extract_urls.h
    http_utils.cpp
    http_utils.h
    link.h
    parse_urls.h
    table_manager.cpp
    table_manager.h
    text_processor.h
    ../config.ini
    ../ini_parser.h
    ../ini_parser.cpp
)

# Установка выходного пути для статических библиотек
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Статическая библиотека inih
add_library(inih STATIC
    ../inih-r58/ini.c
    ../inih-r58/ini.h
    ../inih-r58/cpp/INIReader.cpp
    ../inih-r58/cpp/INIReader.h
)
target_include_directories(inih PUBLIC 
    "${CMAKE_SOURCE_DIR}/../inih-r58"
    "${CMAKE_SOURCE_DIR}/../inih-r58/cpp"
)

# Подключение libpqxx
set(LIBPQXX_DIR "C:/Program Files/pqxx/libpqxx-7.10.0")
add_subdirectory("${LIBPQXX_DIR}" libpqxx-build)

# Настройка MyHTML
set(MYHTML_INSTALL_DIR "C:/Program Files/MyHTML/myhtml-4.0.5/build/install")
set(MYHTML_INCLUDE_DIR "${MYHTML_INSTALL_DIR}/include")
set(MYHTML_LIB_DIR "${MYHTML_INSTALL_DIR}/lib")

# Импорт MyHTML
add_library(myhtml STATIC IMPORTED)
set_target_properties(myhtml PROPERTIES 
    IMPORTED_LOCATION "${MYHTML_LIB_DIR}/myhtml_static.lib"
)

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

# Инклуды для SpiderApp
target_include_directories(SpiderApp PRIVATE 
    ${Boost_INCLUDE_DIRS}
    ${MYHTML_INCLUDE_DIR}
)

# Линковка библиотек
target_link_libraries(SpiderApp PRIVATE 
    inih
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    pqxx
    myhtml
)