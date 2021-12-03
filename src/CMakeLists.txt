set(SRC flow_meta.c packet_processing.c setup.c hairpin.c flow_management.c flow_event.c)

add_executable(smart_offload  main.c ${SRC})

add_library(smart_offload_lib ${SRC})

target_link_libraries(smart_offload ${LIBDPDK_LIBRARIES} Threads::Threads zlog)