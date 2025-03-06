cmake_minimum_required(VERSION 3.20)

cmake_policy(SET CMP0144 NEW)

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
	db_buffer.h
    	db_buffer.cpp
	extract_urls.h
	parse_urls.h
	../config.ini
	../ini_parser.h
	../ini_parser.cpp
	)

target_compile_features(SpiderApp PRIVATE cxx_std_17) 

set(MYHTML_INSTALL_DIR "F:/Projects-2/SPIDER/myhtml-4.0.5/build/install")
set(MYHTML_INCLUDE_DIR "${MYHTML_INSTALL_DIR}/include")
set(MYHTML_LIB_DIR "${MYHTML_INSTALL_DIR}/lib")

add_library(myhtml STATIC IMPORTED)
set_target_properties(myhtml PROPERTIES IMPORTED_LOCATION "${MYHTML_LIB_DIR}/myhtml_static.lib")

add_subdirectory("../../libpqxx-7.10.0" libpqxx-build)

add_library(inih STATIC
    ../inih-r58/ini.c
    ../inih-r58/ini.h
    ../inih-r58/cpp/INIReader.cpp
    ../inih-r58/cpp/INIReader.h
)

target_include_directories(inih PUBLIC "../inih-r58" "../inih-r58/cpp")

set(BOOST_ROOT "../../boost-1.87.0")
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF) 
link_directories("${BOOST_ROOT}/stage/lib")

find_package(Boost 1.87 REQUIRED COMPONENTS locale)
find_package(OpenSSL REQUIRED)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found!")
endif()

target_include_directories(SpiderApp PRIVATE 
	${Boost_INCLUDE_DIRS}
	${MYHTML_INCLUDE_DIR})

target_link_libraries(SpiderApp PRIVATE 
	inih
	${Boost_LIBRARIES} 
	OpenSSL::SSL 
	pqxx
	myhtml)