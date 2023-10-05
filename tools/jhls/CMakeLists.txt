
set(JHLS_SOURCES "jhls.cpp")

add_executable(jhls ${JHLS_SOURCES})
jlm_target_options(jhls)
target_link_libraries(jhls PRIVATE tooling llvm_interface)
