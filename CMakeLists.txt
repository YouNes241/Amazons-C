cmake_minimum_required(VERSION 3.14)
project(AmazonGame LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall -Wextra -g)

# --- TENSORFLOW (Manual download) ---
# --- TENSORFLOW (Téléchargement dynamique) ---
# TODO : When necessary (about 8th or 9th week)

# --- CONFIGURATION DES CHEMINS ---
# 1. Headers
set(TF_INC_PATHS "${TF_DIR}/include" "${TF_DIR}/usr/include" "${TF_DIR}/usr/local/include")
include_directories(${TF_INC_PATHS})

# 2. Librairie (L'étape qui plantait)
message(STATUS "Recherche de la librairie TensorFlow...")

# On définit le dossier exact vu dans tes logs
set(TARGET_LIB_DIR "${TF_DIR}/usr/local/lib")

# ASTUCE : On utilise GLOB pour trouver le fichier RÉEL (avec les numéros de version)
# Cela contourne le problème des liens symboliques cassés
file(GLOB FOUND_LIBS "${TARGET_LIB_DIR}/libtensorflow_framework.so*")

# On prend le premier fichier trouvé qui n'est pas un dossier
foreach(CANDIDATE ${FOUND_LIBS})
    if(NOT IS_DIRECTORY ${CANDIDATE})
        set(TF_LIB_FILE ${CANDIDATE})
        break() # On a trouvé, on arrête
    endif()
endforeach()

# Fallback pour x86 (structure différente)
if(NOT TF_LIB_FILE)
    file(GLOB FOUND_LIBS_X86 "${TF_DIR}/lib/libtensorflow.so*")
    list(GET FOUND_LIBS_X86 0 TF_LIB_FILE)
endif()

# Vérification finale
if(NOT TF_LIB_FILE)
    execute_process(COMMAND find ${TF_DIR} -name "libtensorflow*.so*" OUTPUT_VARIABLE DUMP)
    message(FATAL_ERROR "ECHEC TOTAL. Contenu :\n${DUMP}")
else()
    message(STATUS ">>> SUCCÈS ! Librairie trouvée : ${TF_LIB_FILE}")
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

# --- AUTRES ---
if(DOXYGEN_FOUND)
    add_custom_target(doc COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile)
endif()
option(ENABLE_COVERAGE "Active Gcovr" OFF)
if(ENABLE_COVERAGE)
    add_compile_options(--coverage)
    add_link_options(--coverage)
    add_custom_target(coverage_report COMMAND gcovr -r ${CMAKE_SOURCE_DIR} --html -o coverage.html)
endif()