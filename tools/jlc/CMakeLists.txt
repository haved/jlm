
set(JLC_SOURCES "jlc.cpp")

add_executable(jlc ${JLC_SOURCES})
jlm_target_options(jlc)
target_link_libraries(jlc PRIVATE tooling llvm_interface)
