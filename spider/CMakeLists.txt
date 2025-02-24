cmake_minimum_required(VERSION 3.20)

project(SpiderApp)

set(PostgreSQL_ADDITIONAL_VERSIONS "17")

add_executable(SpiderApp
	SpiderApp.cpp
	http_utils.h
	http_utils.cpp
	link.h
	text_processor.h
	table_manager.h
	table_manager.cpp
	../config.ini
	../inih-r58/ini.c
	../inih-r58/ini.h
	../inih-r58/cpp/INIReader.cpp
	../inih-r58/cpp/INIReader.h
	../ezParser.h
	../ezParser.cpp
	)

add_subdirectory("../../libpqxx-7.10.0" libpqxx-build)
#add_subdirectory("../../boost-1.87.0" libboost-build)
#set(Boost_INCLUDE_DIRS "../../boost-1.87.0")
#set(Boost_LIBRARIES "../../boost-1.87.0/stage/lib")

set(BOOST_ROOT "../../boost-1.87.0")
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF) 
link_directories("${BOOST_ROOT}/stage/lib")

find_package(Boost 1.87 REQUIRED COMPONENTS locale)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found!")
endif()

target_compile_features(SpiderApp PRIVATE cxx_std_17) 

#target_link_libraries(SpiderApp pqxx)

find_package(OpenSSL REQUIRED)
#target_link_libraries(SpiderApp OpenSSL::SSL)

target_include_directories(SpiderApp PRIVATE ${Boost_INCLUDE_DIRS})
#target_link_libraries(SpiderApp ${Boost_LIBRARIES})
target_link_libraries(SpiderApp PRIVATE ${Boost_LIBRARIES} OpenSSL::SSL pqxx)

#include_directories("C:/Program Files/Boost/boost_1_87_0")
#link_directories("C:/Program Files/Boost/boost_1_87_0/stage/lib")
#target_link_libraries(SpiderApp Boost)