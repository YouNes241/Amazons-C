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

# --- TARGETS ---
add_executable(amazons-c src/main.c ${SRC})
target_include_directories(amazons-c PRIVATE ${GUI_INCLUDE_DIRS} "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(amazons-c PRIVATE ${GUI_LIBRARIES} m GTest::GTest)

# Tests
add_executable(unit_tests ${TEST_SOURCES} ${SRC})
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
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E make_directory coverage
        COMMAND ${Python3_EXECUTABLE} -m gcovr -r ${CMAKE_SOURCE_DIR} --html --html-details -o coverage/index.html
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()


