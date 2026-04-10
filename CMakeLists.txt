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
add_compile_options(-Wall -Wextra -g -Werror -Wpedantic -Wvla)

# --- DEPENDENCIES ---
find_package(OpenMP REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GUI REQUIRED gtk4)
pkg_check_modules(READLINE REQUIRED readline)
add_definitions(${GUI_CFLAGS_OTHER})
add_definitions(${READLINE_CFLAGS_OTHER})
find_program(GLIB_COMPILE_RESOURCES NAMES glib-compile-resources REQUIRED)

find_package(Intl)

find_program(XGETTEXT_EXECUTABLE xgettext)
find_program(MSGMERGE_EXECUTABLE msgmerge)
find_program(MSGFMT_EXECUTABLE msgfmt)

if(XGETTEXT_EXECUTABLE AND MSGMERGE_EXECUTABLE AND MSGFMT_EXECUTABLE)
    message(STATUS "I18n tools found. Translation targets enabled.")

    set(PO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/fr.po")
    set(MO_DIR "${CMAKE_CURRENT_BINARY_DIR}/locale/fr/LC_MESSAGES")
    set(MO_FILE "${MO_DIR}/amazons.mo")
    set(POT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/locale/amazons.pot")

    # 1. Sécurité : On ne fait rien si le fichier .po n'est pas là
    if(EXISTS "${PO_FILE}")
        
        # Création du dossier (Configure time) - Ne plante que si disque plein/protégé
        file(MAKE_DIRECTORY "${MO_DIR}")

        # 2. Compilation du .mo (Build time) - RENDU NON-FATAL
        add_custom_command(
            OUTPUT "${MO_FILE}"
            # On utilise une astuce : "commande || cmake -E true" 
            # Cela signifie : "Exécute msgfmt, et si ça rate, exécute une commande qui réussit toujours"
            COMMAND ${MSGFMT_EXECUTABLE} -o "${MO_FILE}" "${PO_FILE}" || ${CMAKE_COMMAND} -E true
            DEPENDS "${PO_FILE}"
            COMMENT "Compiling translations (Non-fatal)"
            VERBATIM
        )

        # 3. La cible ALL dépend maintenant d'une commande qui "réussit" toujours
        add_custom_target(translations ALL DEPENDS "${MO_FILE}")
    else()
        message(STATUS "Warning: fr.po not found at ${PO_FILE}. Translations skipped.")
    endif()

    # 4. Target de mise à jour (Manuelle) - Sécurisée aussi
    file(GLOB_RECURSE I18N_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
    if(I18N_SOURCES)
        add_custom_target(update-po
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/locale"
            COMMAND ${XGETTEXT_EXECUTABLE} --keyword=_ --language=C --from-code=UTF-8 -o ${POT_FILE} ${I18N_SOURCES} || ${CMAKE_COMMAND} -E true
            COMMAND ${MSGMERGE_EXECUTABLE} -U ${PO_FILE} ${POT_FILE} || ${CMAKE_COMMAND} -E true
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Updating .pot (Non-fatal)"
        )
    endif()

else()
    message(STATUS "I18n tools missing. Build will proceed in English only.")
endif()

find_package(Doxygen)

option(ENABLE_ML "Enable Machine Learning (TensorFlow)" OFF)

if(ENABLE_ML)
    FetchContent_Declare(
            tensorflow_c
            URL https://storage.googleapis.com/tensorflow/libtensorflow/libtensorflow-cpu-linux-x86_64-2.15.0.tar.gz
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            DOWNLOAD_NO_PROGRESS FALSE
    )
    FetchContent_MakeAvailable(tensorflow_c)

    set(TF_DIR "${tensorflow_c_SOURCE_DIR}")

    # On garde l'inclusion globale pour la simplicité (Quick Win)
    include_directories(${TF_DIR}/include)
    link_directories(${TF_DIR}/lib)

    include(CheckCSourceRuns)
    set(CMAKE_REQUIRED_INCLUDES "${TF_DIR}/include")
    set(CMAKE_REQUIRED_FLAGS "-L${TF_DIR}/lib")
    set(CMAKE_REQUIRED_LIBRARIES "tensorflow")
    set(CMAKE_REQUIRED_LINK_OPTIONS "-Wl,-rpath,${TF_DIR}/lib")

    set(TF_TEST_CODE "
    #include <stdio.h>
    #include <tensorflow/c/c_api.h>
    int main() {
        printf(\"TF Version: %s\", TF_Version());
        return 0;
    }
    ")

    check_c_source_runs("${TF_TEST_CODE}" TF_IS_USABLE)

    if(TF_IS_USABLE)
        message(STATUS "SUCCESS: TensorFlow C API is fully usable. (please note tests will take longer to run)")
        add_compile_definitions(TF_IS_USABLE)
    endif()
else()
    message(STATUS "ML DISABLED: TensorFlow will not be downloaded or linked.")
endif()

# Force No Optimization for Coverage builds (BRANCHES)
set(CMAKE_C_FLAGS_COVERAGE "-O0 -g --coverage")
set(CMAKE_CXX_FLAGS_COVERAGE "-O0 -g --coverage")

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
        COMMAND ${Python3_EXECUTABLE} -m gcovr 
                -r ${CMAKE_SOURCE_DIR} 
                --object-directory=${CMAKE_BINARY_DIR}
                --filter "${CMAKE_SOURCE_DIR}/src/"
                --filter "${CMAKE_SOURCE_DIR}/include/"
                --html --html-details 
                -o coverage/index.html
                ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating HTML coverage report..."
    )
endif()

if (DOXYGEN_FOUND)
    # set input and output files
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    # request to configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    # Option ALL allows to build the docs together with the application
    add_custom_target( docs ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM )
else (DOXYGEN_FOUND)
  message("Doxygen is required to generate the documentation")
endif (DOXYGEN_FOUND)