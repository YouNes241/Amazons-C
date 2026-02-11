cmake_minimum_required(VERSION 3.14)
project(AmazonGame LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
enable_testing()
add_compile_options(-Wall -Wextra -g)

# --- DEPENDENCIES ---
find_package(OpenMP REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GUI REQUIRED gtk4)
add_definitions(${GUI_CFLAGS_OTHER})
file(COPY "${CMAKE_SOURCE_DIR}/src/view/ui" DESTINATION "${CMAKE_BINARY_DIR}")

# System GTest
find_package(GTest REQUIRED)

find_package(Intl)
find_package(Doxygen)

# --- SUBDIRECTORIES ---
add_subdirectory(src)
add_subdirectory(tests)

# --- COVERAGE ---
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Coverage")
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_compile_options(--coverage)
    add_link_options(--coverage)
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E make_directory coverage
        COMMAND ${Python3_EXECUTABLE} -m gcovr -r ${CMAKE_SOURCE_DIR} --html --html-details -o coverage/index.html
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()


#TEMP test network TODO
# --- TESTS RÉSEAU (C PUR) ---
add_executable(test_network_buffer tests/network/test_buffer_tcp.c src/network/tcp_buffer.c)
target_include_directories(test_network_buffer PRIVATE "${CMAKE_SOURCE_DIR}/include")
add_test(NAME TestBufferTCP COMMAND test_network_buffer)
