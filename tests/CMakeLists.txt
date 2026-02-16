# --- GATHER TESTS ---
file(GLOB_RECURSE TEST_SOURCES "*.cpp")

# --- CREATE TEST EXECUTABLE ---
# IMPORTANT: We include {GAME_SRC} here! 
# This variable comes from src/CMakeLists.txt (if you used the code I gave you).
add_executable(unit_tests ${TEST_SOURCES} ${GAME_SRC})

# --- INCLUDES ---
target_include_directories(unit_tests PRIVATE 
    "${CMAKE_SOURCE_DIR}/include"
    ${GUI_INCLUDE_DIRS}
)

# --- LINKING ---
# 1. Link OpenMP (Critical for Heuristics)
if(OpenMP_C_FOUND)
    target_link_libraries(unit_tests PRIVATE OpenMP::OpenMP_C)
endif()

# 2. Link GTest and GTK
target_link_libraries(unit_tests PRIVATE 
    GTest::GTest 
    GTest::Main 
    ${GUI_LIBRARIES} 
    m
)

# --- DISCOVER TESTS ---
include(GoogleTest)
gtest_discover_tests(unit_tests)