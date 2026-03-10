cmake_minimum_required(VERSION 3.14)
project(AmazonGame LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        DOWNLOAD_NO_PROGRESS TRUE
      )
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(CTest)
include(GoogleTest)

enable_testing()
add_compile_options(-Wall -Wextra -g)

# --- DEPENDENCIES ---
find_package(OpenMP REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GUI REQUIRED gtk4)
pkg_check_modules(READLINE REQUIRED readline)
add_definitions(${GUI_CFLAGS_OTHER})
add_definitions(${READLINE_CFLAGS_OTHER})
find_program(GLIB_COMPILE_RESOURCES NAMES glib-compile-resources REQUIRED)

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

# --- TEST BITBOARD (TEMP) ---
add_executable(test_bitboard tests/test_bitboard.c)
target_include_directories(test_bitboard PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src/gamemodel")
target_link_libraries(test_bitboard PRIVATE amazons_core)
add_test(NAME TestBitboard COMMAND test_bitboard)

# --- TESTS GUI (TEMP) ---
add_executable(test_gui_application tests/test_gui_application.c)
target_include_directories(test_gui_application PRIVATE "${CMAKE_SOURCE_DIR}/include" ${GUI_INCLUDE_DIRS} ${READLINE_INCLUDE_DIRS})
target_link_libraries(test_gui_application PRIVATE amazons_core ${GUI_LIBRARIES} ${READLINE_LIBRARIES})