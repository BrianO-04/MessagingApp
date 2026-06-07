cmake_minimum_required(VERSION 3.10.0)
project(MessagingApp VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(WIN32)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
include_directories(lib)
endif()

include_directories(include)
include_directories(src)

add_executable(MessagingApp 
    src/cursesUI.c
    src/messagelog.c
    src/aes.c
    src/macros.c
    src/client.c)

add_executable(MessagingServer 
    src/messagelog.c
    src/aes.c
    src/macros.c
    src/hashmap.c
    src/server.c)

if(WIN32)
    target_link_libraries(MessagingApp ws2_32)
    target_link_libraries(MessagingServer ws2_32)
    
    target_link_directories(MessagingApp PRIVATE ${CMAKE_SOURCE_DIR}/lib)
    target_include_directories(MessagingApp PRIVATE ${CMAKE_SOURCE_DIR}/lib)
    target_link_libraries(MessagingApp pdcurses)
else()
    target_link_libraries(MessagingApp ncurses)
endif()

