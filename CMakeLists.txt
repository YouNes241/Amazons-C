cmake_minimum_required(VERSION 3.14)
project(AmazonGame LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall -Wextra -g)

# --- TENSORFLOW (Le seul qui reste manuel) ---
# Même avec sudo, TensorFlow C API n'est pas dans les paquets Ubuntu par défaut.
# On garde donc ce bloc, OU on l'installe manuellement dans /usr/local/lib une bonne fois pour toutes.
set(TF_VER "2.14.0")
set(TF_DIR "${CMAKE_SOURCE_DIR}/libs/libtensorflow")
if(NOT EXISTS ${TF_DIR})
    message(STATUS "Downloading TensorFlow...")
    file(DOWNLOAD "https://storage.googleapis.com/tensorflow/libtensorflow/libtensorflow-gpu-linux-x86_64-${TF_VER}.tar.gz" libs/tf.tar.gz)
    file(MAKE_DIRECTORY ${TF_DIR})
    execute_process(COMMAND tar -xzf libs/tf.tar.gz -C ${TF_DIR})
    file(REMOVE libs/tf.tar.gz)
endif()
include_directories(${TF_DIR}/include)
link_directories(${TF_DIR}/lib)

# --- DÉPENDANCES SYSTÈME (Tout est automatique via find_package) ---

# 1. GTK (Plus besoin de fallback, on a installé la libgtk-4-dev)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GUI REQUIRED gtk4)
add_definitions(${GUI_CFLAGS_OTHER})

# 2. GoogleTest (Installé via apt dans /usr/src/googletest)
find_package(GTest REQUIRED)

# 3. Autres
find_package(Intl)
find_package(Doxygen)

# --- SOURCES ---
file(GLOB_RECURSE SRC "src/*.c")
list(REMOVE_ITEM SRC "${CMAKE_SOURCE_DIR}/src/main.c")

# --- CIBLES ---
add_executable(amazon_game src/main.c ${SRC})
target_include_directories(amazon_game PRIVATE ${GUI_INCLUDE_DIRS})
target_link_libraries(amazon_game PRIVATE ${GUI_LIBRARIES} tensorflow m GTest::GTest)

# Tests
add_executable(unit_tests tests/test_main.cpp ${SRC})
target_link_libraries(unit_tests PRIVATE GTest::GTest GTest::Main tensorflow m)
gtest_discover_tests(unit_tests)

# --- DOC & COVERAGE ---
if(DOXYGEN_FOUND)
    add_custom_target(doc COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile)
endif()

# Gcovr est supposé être dans le PATH système (/usr/bin/gcovr)
option(ENABLE_COVERAGE "Active Gcovr" OFF)
if(ENABLE_COVERAGE)
    add_compile_options(--coverage)
    add_link_options(--coverage)
    add_custom_target(coverage_report COMMAND gcovr -r ${CMAKE_SOURCE_DIR} --html --html-details -o coverage.html)
endif()
