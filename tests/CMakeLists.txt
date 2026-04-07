# --- GATHER TESTS ---
file(GLOB_RECURSE TEST_SOURCES "*.cpp")

# --- CREATE TEST-SPECIFIC MINIMAX LIBRARY ---
# Build minimax with MINIMAX_TEST_MODE enabled to expose internal functions
add_library(minimax_test_lib OBJECT "${CMAKE_SOURCE_DIR}/src/ai/minimax/minimax.c")
target_include_directories(minimax_test_lib PRIVATE 
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/src"
    ${GUI_INCLUDE_DIRS}
)
target_compile_definitions(minimax_test_lib PRIVATE "MINIMAX_TEST_MODE")

# --- CREATE TEST-SPECIFIC MCTS LIBRARY ---
# Build MCTS with MCTS_TEST_MODE enabled to expose internal functions
add_library(mcts_test_lib OBJECT "${CMAKE_SOURCE_DIR}/src/ai/mcts/mcts.c")
target_include_directories(mcts_test_lib PRIVATE 
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/src"
    ${GUI_INCLUDE_DIRS}
)
target_compile_definitions(mcts_test_lib PRIVATE "MCTS_TEST_MODE")
# Add OpenMP support
if(OpenMP_C_FOUND)
    target_link_libraries(mcts_test_lib PRIVATE OpenMP::OpenMP_C)
endif()

# --- CREATE TEST EXECUTABLE ---
add_executable(unit_tests ${TEST_SOURCES} $<TARGET_OBJECTS:minimax_test_lib> $<TARGET_OBJECTS:mcts_test_lib>)

# --- INCLUDES ---
target_include_directories(unit_tests PRIVATE 
    "${CMAKE_SOURCE_DIR}/include"
    ${GUI_INCLUDE_DIRS}
    ${READLINE_INCLUDE_DIRS}
)
target_compile_definitions(amazons_core PUBLIC UNIT_TESTING)
target_compile_definitions(unit_tests PRIVATE UNIT_TESTING)

# --- LINKING ---
#OpenMP
if(OpenMP_C_FOUND)
    target_link_libraries(unit_tests PRIVATE OpenMP::OpenMP_C)
endif()

# GTest
target_link_libraries(unit_tests PRIVATE 
    amazons_core
    GTest::gtest_main 
    ${GUI_LIBRARIES} 
    ${READLINE_LIBRARIES}
    m
)

# Allow test version of minimax to override the one from amazons_core
target_link_options(unit_tests PRIVATE "LINKER:--allow-multiple-definition")
include(GoogleTest)

# Set the compiler flags on the Coverage build-type
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS_DEBUG} --coverage")
  set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage")
  set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} --coverage")
  set(CMAKE_MODULE_LINKER_FLAGS_COVERAGE "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} --coverage")
endif()

# --- DISCOVER TESTS ---
gtest_discover_tests(unit_tests)