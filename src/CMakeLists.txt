set(SRC flow_meta.c packet_processing.c setup.c hairpin.c flow_management.c flow_event.c)

add_library(smart_offload_lib ${SRC})
add_dependencies(smart_offload_lib zlog)
target_link_libraries(smart_offload_lib zlog ${LIBDPDK_LIBRARIES} Threads::Threads zlog)

add_executable(smart_offload main.c)
target_link_libraries(smart_offload smart_offload_lib)