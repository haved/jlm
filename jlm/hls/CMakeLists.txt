set(LIBHLS_SOURCES
    "backend/rvsdg2rhls/add-triggers.cpp"
    "backend/rvsdg2rhls/gamma-conv.cpp"
    "backend/rvsdg2rhls/theta-conv.cpp"
    "backend/rvsdg2rhls/add-sinks.cpp"
    "backend/rvsdg2rhls/add-forks.cpp"
    "backend/rvsdg2rhls/check-rhls.cpp"
    "backend/rvsdg2rhls/rhls-dne.cpp"
    "backend/rvsdg2rhls/remove-unused-state.cpp"
    "backend/rvsdg2rhls/rvsdg2rhls.cpp"
    "backend/rvsdg2rhls/add-prints.cpp"
    "backend/rvsdg2rhls/add-buffers.cpp"

    "backend/rhls2firrtl/base-hls.cpp"
    "backend/rhls2firrtl/dot-hls.cpp"
    "backend/rhls2firrtl/verilator-harness-hls.cpp"
    "backend/rhls2firrtl/firrtl-hls.cpp"
    "backend/rhls2firrtl/mlirgen.cpp"

    "ir/hls.cpp")

add_library(hls STATIC ${LIBHLS_SOURCES})
jlm_target_options(hls)
target_link_libraries(hls PRIVATE rvsdg util)

# TODO: We probably need to add some CIRCT things here
