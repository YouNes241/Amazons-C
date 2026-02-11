cmake_minimum_required(VERSION 3.14)
project(AmazonGame LANGUAGES C CXX)
enable_testing()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall -Wextra -g)

# --- TENSORFLOW (Manual download) ---
# --- TENSORFLOW (Téléchargement dynamique) ---
# TODO : When necessary (about 8th or 9th week)

include_directories(${TF_DIR}/include)
link_directories(${TF_DIR}/lib)
# --- System dependencies - should be downloaded prior using sudo apt-get ---

# 1. GTK
find_package(PkgConfig REQUIRED)
pkg_check_modules(GUI REQUIRED gtk4)
add_definitions(${GUI_CFLAGS_OTHER})

# 2. GoogleTest
find_package(GTest REQUIRED)

# 3. Others
find_package(Intl)
find_package(Doxygen)

# --- SOURCES ---
file(GLOB_RECURSE SRC "src/*.c")
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
list(REMOVE_ITEM SRC "${CMAKE_SOURCE_DIR}/src/main.c")

# --- TARGETS ---
add_executable(amazons-c src/main.c ${SRC})
target_include_directories(amazons-c PRIVATE ${GUI_INCLUDE_DIRS} "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(amazons-c PRIVATE ${GUI_LIBRARIES} m GTest::GTest)

# Tests
add_executable(unit_tests ${TEST_SOURCES})
target_include_directories(unit_tests PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(unit_tests PRIVATE GTest::GTest GTest::Main m)
gtest_discover_tests(unit_tests)

# --- DOC & COVERAGE ---
if(DOXYGEN_FOUND)
    add_custom_target(doc COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile)
endif()

# Gcovr for code coverage
option(ENABLE_COVERAGE "Active Gcovr" OFF)
if(ENABLE_COVERAGE)
    add_compile_options(--coverage)
    add_link_options(--coverage)
    add_custom_target(coverage_report 
        COMMAND gcovr -r ${CMAKE_SOURCE_DIR} --html --html-details -o coverage.html
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()


#TEMP test network TODO
# --- TESTS RÉSEAU (C PUR) ---
add_executable(test_network_buffer tests/test_buffer_tcp.c src/network/tcp_buffer.c)
target_include_directories(test_network_buffer PRIVATE "${CMAKE_SOURCE_DIR}/include")
add_test(NAME TestBufferTCP COMMAND test_network_buffer)