cmake_minimum_required(VERSION 3.14)
project(AmazonGame LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall -Wextra -g)

# --- TENSORFLOW (Manual download) ---
# --- TENSORFLOW (Téléchargement dynamique) ---
# TODO : When necessary (about 8th or 9th week)

if(EXISTS "${TF_DIR}/include/tensorflow")
    include_directories(${TF_DIR}/include)
elseif(EXISTS "${TF_DIR}/usr/local/include/tensorflow")
    include_directories(${TF_DIR}/usr/local/include)
elseif(EXISTS "${TF_DIR}/usr/include/tensorflow")
    include_directories(${TF_DIR}/usr/include)
endif()

set(TF_SEARCH_PATHS 
    ${TF_DIR}/lib 
    ${TF_DIR}/usr/lib 
    ${TF_DIR}/usr/local/lib
    ${TF_DIR}/usr/lib/aarch64-linux-gnu
)

link_directories(${TF_SEARCH_PATHS})

find_library(TF_LIB_FILE 
    NAMES tensorflow tensorflow_framework 
    PATHS ${TF_SEARCH_PATHS}
    NO_DEFAULT_PATH
)

if(NOT TF_LIB_FILE)
    execute_process(COMMAND find ${TF_DIR} -maxdepth 4 -name "libtensorflow*" OUTPUT_VARIABLE FOUND_FILES)
    message(WARNING "Fichiers trouvés dans TF_DIR : \n${FOUND_FILES}")
    message(FATAL_ERROR "Impossible de trouver tensorflow dans ${TF_DIR} (voir les chemins testés ci-dessus)")
else()
    message(STATUS "TensorFlow trouvé ici : ${TF_LIB_FILE}")
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
<<<<<<< HEAD
add_executable(amazons-c src/main.c ${SRC})
target_include_directories(amazons-c PRIVATE ${GUI_INCLUDE_DIRS})
target_link_libraries(amazons-c PRIVATE ${GUI_LIBRARIES} m GTest::GTest)
=======
add_executable(amazon_game src/main.c ${SRC})
target_include_directories(amazon_game PRIVATE ${GUI_INCLUDE_DIRS})
target_link_libraries(amazon_game PRIVATE ${GUI_LIBRARIES} ${TF_LIB_FILE} m GTest::GTest)
>>>>>>> 4ca33d3 (fix: Attempting fix so the project can build on CI/CD)

# Tests
add_executable(unit_tests tests/test_main.cpp ${SRC})
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
