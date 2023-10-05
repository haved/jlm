
set(JLMOPT_SOURCES "jlm-opt.cpp")

add_executable(jlm-opt ${JLMOPT_SOURCES})
jlm_target_options(jlm-opt)
target_link_libraries(jlm-opt PRIVATE tooling llvm_interface)
