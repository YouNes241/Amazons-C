# --- GATHER TESTS ---
file(GLOB_RECURSE TEST_SOURCES "*.cpp")

# --- CREATE TEST EXECUTABLE ---
add_executable(unit_tests ${TEST_SOURCES} ${GAME_SRC})

# --- INCLUDES ---
target_include_directories(unit_tests PRIVATE 
    "${CMAKE_SOURCE_DIR}/include"
    ${GUI_INCLUDE_DIRS}
    ${READLINE_INCLUDE_DIRS}
)

# --- LINKING ---
#OpenMP
if(OpenMP_C_FOUND)
    target_link_libraries(unit_tests PRIVATE OpenMP::OpenMP_C)
endif()

# GTest
target_link_libraries(unit_tests PRIVATE 
    GTest::gtest_main 
    ${GUI_LIBRARIES} 
    ${READLINE_LIBRARIES}
    m
)
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