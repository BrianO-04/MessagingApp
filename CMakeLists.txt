cmake_minimum_required(VERSION 3.10.0)
project(MessagingApp VERSION 0.1.0 LANGUAGES C)

include_directories(include)
include_directories(src)
add_executable(MessagingApp src/client.c)
add_executable(MessagingServer src/server.c)

