cmake_minimum_required(VERSION 3.20)

project(MyProjectSpider)

set(PostgreSQL_ADDITIONAL_VERSIONS "17")

add_executable(MyProjectSpider 
	ProjectSpider.cpp
	ezParser.h
        ezParser.cpp
        config.ini
 	)

add_subdirectory("../libpqxx-7.10.0" libpqxx-build)

target_compile_features(MyProjectSpider PRIVATE cxx_std_17)

target_link_libraries(MyProjectSpider pqxx)