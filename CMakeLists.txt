cmake_minimum_required(VERSION 3.10)
project(amazons-c C)

# 1. Conformité Système et Flags (Exigences 1.5, 1.7, 1.11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -g --coverage")

# 2. Localisation des Headers (Exigence 1.9)
include_directories(include)

# 3. Recherche des dépendances obligatoires
find_package(PkgConfig REQUIRED)

# LibGtk 4.x (Exigence 1.13)
pkg_check_modules(GTK4 REQUIRED gtk4)

# TensorFlow C API (Exigence 1.15) - On suppose qu'elle est installée sur le système
find_library(TF_LIB tensorflow)

# Gettext pour l'internationalisation (Exigence 1.14)
find_package(Intl REQUIRED)

# 4. Collecte des fichiers sources (Exigence 1.10)
# On cherche récursivement dans src/ pour inclure ai, gamemodel, network, etc.
file(GLOB_RECURSE SOURCES "src/*.c")

# 5. Création de l'exécutable
add_executable(${PROJECT_NAME} ${SOURCES})

# 6. Linkage des bibliothèques (Exigences 1.12, 1.13, 1.14, 1.15)
target_link_libraries(${PROJECT_NAME} 
    ${GTK4_LIBRARIES} 
    ${TF_LIB} 
    ${Intl_LIBRARIES}
    m               # Math library
)

# 7. Framework de Test (Exigences 1.6, 1.11)
enable_testing()
find_package(GTest REQUIRED)
# Ici on exclut main.c pour éviter les conflits de main() dans les tests
file(GLOB_RECURSE TEST_SOURCES "tests/*.c")
add_executable(unit_tests ${TEST_SOURCES} ${filter_out_main_sources}) 
add_test(NAME AllTests COMMAND unit_tests)
