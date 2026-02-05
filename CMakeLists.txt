cmake_minimum_required(VERSION 3.14)
project(AmazonGame LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall -Wextra -g)

# --- TENSORFLOW (Manual download) ---
option(ENABLE_GPU "TensorFlow GPU" OFF)

if(ENABLE_GPU)
    set(TF_TYPE "gpu")
    message(STATUS "Mode TensorFlow : GPU")
else()
    set(TF_TYPE "cpu")
    message(STATUS "Mode TensorFlow : CPU")
endif()

# --- TENSORFLOW (Téléchargement dynamique) ---
set(TF_VER "2.14.0")
set(TF_DIR "${CMAKE_SOURCE_DIR}/libs/libtensorflow")
set(TF_MARKER "${TF_DIR}/installed_${TF_TYPE}.marker")

if(NOT EXISTS ${TF_MARKER})
    message(STATUS "Téléchargement de TensorFlow (${TF_TYPE})...")
    file(REMOVE_RECURSE ${TF_DIR})
    file(MAKE_DIRECTORY ${TF_DIR})
    
    file(DOWNLOAD "https://storage.googleapis.com/tensorflow/libtensorflow/libtensorflow-${TF_TYPE}-linux-x86_64-${TF_VER}.tar.gz" libs/tf.tar.gz SHOW_PROGRESS)
    execute_process(COMMAND tar -xzf libs/tf.tar.gz -C ${TF_DIR})
    file(REMOVE libs/tf.tar.gz)
    
    file(WRITE ${TF_MARKER} "Installed")
endif()

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
list(REMOVE_ITEM SRC "${CMAKE_SOURCE_DIR}/src/main.c")

# --- TARGETS ---
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
