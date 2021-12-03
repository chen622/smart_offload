
add_executable(test-flow flow.c)

target_link_libraries(test-flow ${LIBDPDK_LIBRARIES} Threads::Threads zlog smart_offload_lib)