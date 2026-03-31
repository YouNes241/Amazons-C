# --- 1. FIND SOURCES CORRECTLY ---
file(GLOB_RECURSE ALL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

# --- GRESOURCE (GTK) ---
set(GRESOURCE_XML "${CMAKE_CURRENT_SOURCE_DIR}/view/ui/gresource.xml")
set(GRESOURCE_BIN "${CMAKE_BINARY_DIR}/resources.gresource")
add_custom_command(
    OUTPUT ${GRESOURCE_BIN}
    COMMAND ${GLIB_COMPILE_RESOURCES}
            --sourcedir="${CMAKE_CURRENT_SOURCE_DIR}/view/ui"
            --target=${GRESOURCE_BIN}
            ${GRESOURCE_XML}
    DEPENDS ${GRESOURCE_XML}
)
add_custom_target(ui_resources ALL DEPENDS ${GRESOURCE_BIN})

# --- 2. PREPARE EXPORTS FOR TESTS ---
set(MAIN_SRC "${CMAKE_CURRENT_SOURCE_DIR}/main.c")
set(GAME_LOGIC ${ALL_SRC})
list(REMOVE_ITEM GAME_LOGIC ${MAIN_SRC})
add_library(amazons_core OBJECT ${GAME_LOGIC})

# --- 3. BUILD THE MAIN GAME EXECUTABLE ---
add_executable(amazons ${MAIN_SRC})
target_link_libraries(amazons PRIVATE amazons_core)

set_target_properties(amazons PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# --- 4. COMPILER FLAGS & LINKING ---
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS_DEBUG} --coverage")
  set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage")
  set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} --coverage")
  set(CMAKE_MODULE_LINKER_FLAGS_COVERAGE "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} --coverage")
endif()

link_directories(${CMAKE_SOURCE_DIR}/lib)

# Link Dependencies
target_include_directories(amazons_core PUBLIC ${GUI_INCLUDE_DIRS} ${READLINE_INCLUDE_DIRS} "${CMAKE_SOURCE_DIR}/include")

if(OpenMP_C_FOUND)
    target_link_libraries(amazons_core PUBLIC OpenMP::OpenMP_C)
endif()

target_link_libraries(amazons_core PUBLIC ${GUI_LIBRARIES} ${READLINE_LIBRARIES} m) 