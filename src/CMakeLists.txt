set(SRC main.c flow_meta.c packet_processing.c setup.c hairpin.c)

add_executable(smart_offload ${SRC})

target_link_libraries(smart_offload PRIVATE ${LIBDPDK_LIBRARIES} Threads::Threads zlog)