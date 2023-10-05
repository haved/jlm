
# Due to dependencies between static constructors, these object files must precede the tests
set(TEST_HARNESS_SOURCES
        "test-operation.cpp"
        "test-registry.cpp"
        "test-runner.cpp"
        "test-types.cpp"
        "TestRvsdgs.cpp")
file(GLOB_RECURSE TEST_SOURCES "jlm/*.cpp")

add_executable(test-runner ${TEST_HARNESS_SOURCES} ${TEST_SOURCES})
target_include_directories(test-runner PRIVATE "${CMAKE_SOURCE_DIR}/tests")

# NOTE: This enables asserts in Release builds, but only in test files.
# To get asserts in all files, build tests with CMAKE_BUILD_TYPE=Debug
target_compile_definitions(test-runner PRIVATE -DJLM_DEBUG -DJLM_ENABLE_ASSERTS)

# The normal flags passed to all executables
jlm_target_options(test-runner)
target_link_libraries(test-runner PRIVATE tooling hls llvm rvsdg util llvm_interface)
