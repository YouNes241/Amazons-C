cmake_minimum_required(VERSION 3.14)
project(AmazonGame LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall -Wextra -g)

# --- TENSORFLOW (Manual download) ---
# --- TENSORFLOW (Téléchargement dynamique) ---
# TODO : When necessary (about 8th or 9th week)

# --- DÉTECTION DES CHEMINS (C'est là que ça plantait) ---

# 1. Gestion des Headers (.h)
# On ajoute tous les dossiers 'include' potentiels trouvés dans l'archive
set(TF_INC_PATHS
    "${TF_DIR}/include"
    "${TF_DIR}/usr/include"
    "${TF_DIR}/usr/local/include"
)
include_directories(${TF_INC_PATHS})

# 2. Gestion de la Librairie (.so)
# On pointe spécifiquement là où tes logs ont montré le fichier
set(TF_LIB_SEARCH_PATH "${TF_DIR}/usr/local/lib")

# On cherche 'tensorflow_framework' car 'tensorflow' n'existe pas dans ce paquet ARM
find_library(TF_LIB_FILE 
    NAMES tensorflow_framework tensorflow
    PATHS ${TF_LIB_SEARCH_PATH} "${TF_DIR}/lib"
    NO_DEFAULT_PATH
)

# SECOURS : Si find_library échoue encore, on force le chemin manuellement
if(NOT TF_LIB_FILE)
    if(EXISTS "${TF_LIB_SEARCH_PATH}/libtensorflow_framework.so")
        set(TF_LIB_FILE "${TF_LIB_SEARCH_PATH}/libtensorflow_framework.so")
        message(STATUS "TensorFlow forcé manuellement : ${TF_LIB_FILE}")
    endif()
endif()

# VERDICT FINAL
if(NOT TF_LIB_FILE)
    execute_process(COMMAND find ${TF_DIR} -name "libtensorflow*.so*" OUTPUT_VARIABLE DUMP)
    message(FATAL_ERROR "Echec critique. Fichiers présents :\n${DUMP}")
else()
    message(STATUS "TensorFlow Library OK : ${TF_LIB_FILE}")
endif()

# --- DÉPENDANCES SYSTÈME ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(GUI REQUIRED gtk4)
add_definitions(${GUI_CFLAGS_OTHER})
find_package(GTest REQUIRED)
find_package(Intl)
find_package(Doxygen)

# --- CIBLES ---
file(GLOB_RECURSE SRC "src/*.c")
list(REMOVE_ITEM SRC "${CMAKE_SOURCE_DIR}/src/main.c")

<<<<<<< HEAD
# --- TARGETS ---
<<<<<<< HEAD
add_executable(amazons-c src/main.c ${SRC})
target_include_directories(amazons-c PRIVATE ${GUI_INCLUDE_DIRS})
target_link_libraries(amazons-c PRIVATE ${GUI_LIBRARIES} m GTest::GTest)
=======
=======
# Jeu
>>>>>>> 6094a29 (fix: Attempting fix so the project can build on CI/CD)
add_executable(amazon_game src/main.c ${SRC})
target_include_directories(amazon_game PRIVATE ${GUI_INCLUDE_DIRS})
# On linke le fichier exact trouvé (.so), pas juste le nom
target_link_libraries(amazon_game PRIVATE ${GUI_LIBRARIES} ${TF_LIB_FILE} m GTest::GTest)
>>>>>>> 4ca33d3 (fix: Attempting fix so the project can build on CI/CD)

# Tests
add_executable(unit_tests tests/test_main.cpp ${SRC})
<<<<<<< HEAD
target_link_libraries(unit_tests PRIVATE GTest::GTest GTest::Main m)
=======
target_link_libraries(unit_tests PRIVATE GTest::GTest GTest::Main ${TF_LIB_FILE} m)
>>>>>>> 6094a29 (fix: Attempting fix so the project can build on CI/CD)
gtest_discover_tests(unit_tests)

# --- DOC & COVERAGE ---
if(DOXYGEN_FOUND)
    add_custom_target(doc COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile)
endif()
option(ENABLE_COVERAGE "Active Gcovr" OFF)
if(ENABLE_COVERAGE)
    add_compile_options(--coverage)
    add_link_options(--coverage)
    add_custom_target(coverage_report COMMAND gcovr -r ${CMAKE_SOURCE_DIR} --html -o coverage.html)
endif()