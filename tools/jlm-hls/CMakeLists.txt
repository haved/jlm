
set(JLMHLS_SOURCES "jlm-hls.cpp")

add_executable(jlm-hls ${JLMHLS_SOURCES})
jlm_target_options(jlm-hls)
target_link_libraries(jlm-hls PRIVATE tooling llvm_interface)

# TODO: Flags get changed, and libraries added, when using CIRCT.
# I'm not entirely sure which targets need their flags changed.
# It still links completely fine without it.
