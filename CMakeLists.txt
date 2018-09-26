set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
cmake_minimum_required(VERSION 3.0.0)
project(share)

include_directories(./src/tor/util
                    ./src/tor/socks
                    ./src/tor/controller
                    ./src/tor/host)

add_executable(share ./src/main.c 
                    ./src/tor/controller/tor_controller.c
                    ./src/tor/controller/tor_exe.c
                    ./src/tor/controller/tor_service.c
                    ./src/tor/host/host.c
                    ./src/tor/socks/socks.c
                    ./src/tor/util/log.c
                    ./src/tor/util/socket.c
                    ./src/tor/util/tor_util.c)

if(WIN32)                
target_link_libraries(share Ws2_32.lib
                            Mswsock.lib
                            AdvApi32.lib
                            psapi.lib)
endif(WIN32)


