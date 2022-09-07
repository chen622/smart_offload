add_executable(smto-test smto_test.c)
add_dependencies(smto-test zlog smart_offload_lib)
target_link_libraries(smto-test ${LIBDPDK_LIBRARIES} Threads::Threads zlog smart_offload_lib)

add_executable(test-flow flow.c)
add_dependencies(test-flow zlog smart_offload_lib)
target_link_libraries(test-flow ${LIBDPDK_LIBRARIES} Threads::Threads zlog smart_offload_lib)

add_executable(test-mem mem.c)
add_dependencies(test-mem zlog smart_offload_lib)
target_link_libraries(test-mem ${LIBDPDK_LIBRARIES} Threads::Threads zlog smart_offload_lib)