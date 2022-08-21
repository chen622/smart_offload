set(SRC smto.c smto_common.c smto_setup.c smto_flow_engine.c smto_flow_key.c smto_event.c smto_worker.c)

add_library(smart_offload_lib ${SRC})
add_dependencies(smart_offload_lib rdarm)
#target_link_libraries(smart_offload_lib zlog ${LIBDPDK_LIBRARIES} Threads::Threads)

#add_executable(smart_offload v1/main.c)
#add_dependencies(smart_offload zlog smart_offload_lib)
#target_link_libraries(smart_offload smart_offload_lib zlog ${LIBDPDK_LIBRARIES} Threads::Threads)