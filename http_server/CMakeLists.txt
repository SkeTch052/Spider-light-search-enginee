cmake_minimum_required(VERSION 3.20)

project (HttpServerApp)

set(PostgreSQL_ADDITIONAL_VERSIONS "17")

add_executable(HttpServerApp 
	HttpServerApp.cpp
	http_connection.h
	http_connection.cpp
	search_documents.h
	search_documents.cpp
	../config.ini
	../inih-r58/ini.c
	../inih-r58/ini.h
	../inih-r58/cpp/INIReader.cpp
	../inih-r58/cpp/INIReader.h
	../ezParser.h
	../ezParser.cpp
	)

target_compile_features(HttpServerApp PRIVATE cxx_std_17) 

add_subdirectory("../../libpqxx-7.10.0" libpqxx-build)

set(BOOST_ROOT "../../boost-1.87.0")
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF) 

find_package(Boost 1.87 REQUIRED)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found!")
endif()

#target_link_libraries(HttpServerApp pqxx)  

target_include_directories(HttpServerApp PRIVATE ${Boost_INCLUDE_DIRS})

#target_link_libraries(HttpServerApp ${Boost_LIBRARIES})

include_directories("C:/Program Files/Boost/boost_1_87_0")
link_directories("C:/Program Files/Boost/boost_1_87_0/stage/lib")

find_package(OpenSSL REQUIRED)
target_link_libraries(HttpServerApp PRIVATE ${Boost_LIBRARIES} OpenSSL::SSL pqxx)

