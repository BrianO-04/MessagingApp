cmake_minimum_required(VERSION 3.10.0)
project(MessagingApp VERSION 0.1.0 LANGUAGES C)

include_directories(include)
include_directories(src)

add_executable(MessagingApp src/client.c)

add_executable(MessagingServer 
    src/hashmap.c
    src/server.c)

include(CheckLibraryExists)
check_library_exists(m pow "" LIBM_FOUND)

if(LIBM_FOUND)
    target_link_libraries(MessagingServer PRIVATE m)
endif()

