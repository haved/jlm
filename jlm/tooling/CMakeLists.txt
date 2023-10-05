set(LIBTOOLING_SOURCES
    "Command.cpp"
    "CommandGraph.cpp"
    "CommandGraphGenerator.cpp"
    "CommandLine.cpp")

# TODO: Add generation of CommandPaths.hpp
# For now run make jlm/tooling/CommandPaths.hpp manually

add_library(tooling STATIC ${LIBTOOLING_SOURCES})
jlm_target_options(tooling)
target_link_libraries(tooling PRIVATE hls llvm rvsdg util llvm_interface)
