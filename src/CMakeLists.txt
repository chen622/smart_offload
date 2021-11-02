set(SRC main.c packet_processing.c setup.c)

add_executable(smart_offload ${SRC})

target_link_libraries(smart_offload PRIVATE ${LIBDPDK_LIBRARIES} Threads::Threads zlog)