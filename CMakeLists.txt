cmake_minimum_required(VERSION 3.2)
project(ip_filter VERSION 0.0.$ENV{TRAVIS_BUILD_NUMBER})

set(SOURCES
	ip_filter.cpp
	)
set(HEADERS_LIB
	ip_filter_lib.h
	utils.h
	)
set(SOURCES_LIB
	ip_filter_lib.cpp
	utils.cpp
	)
set(DATA_FILES
	Readme.md
	)
option(OPTIMIZE_MEMORY "Enable memory optimization" true)
option(USE_VECTOR "Enable using vector for ip_pool" false)
option(USE_LIST "Enable using list for ip_pool" false)
configure_file(config.h.in config.h)
add_executable(ip_filter ${SOURCES} ${HEADERS_LIB} ${DATA_FILES})
add_library(ip_filter_lib STATIC ${SOURCES_LIB} ${HEADERS_LIB})
target_link_libraries(ip_filter ip_filter_lib)
set_target_properties(ip_filter ip_filter_lib PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
)

### tests ###
add_executable(tests tests.cpp ${HEADERS_LIB})
find_package(Boost COMPONENTS unit_test_framework filesystem REQUIRED)
set_target_properties(tests PROPERTIES
	COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK
	INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIRS}
	LINK_DIRECTORIES ${Boost_LIBRARY_DIRS}
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
)
target_link_libraries(tests
  ${Boost_LIBRARIES}
  ip_filter_lib
  )
enable_testing()
add_test(ip_filter_tests tests)
###

### Packaging ###
install(TARGETS ip_filter RUNTIME DESTINATION bin)
install(TARGETS tests RUNTIME DESTINATION lib/ip_filter/tests/build/bin)
install(FILES ip_filter.tsv ip_filter.tst DESTINATION lib/ip_filter/tests)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT linkfly@yandex.ru)

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)

