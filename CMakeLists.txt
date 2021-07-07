cmake_minimum_required(VERSION 3.12)
project(spine)

include(GNUInstallDirs)

add_definitions(-DZENOH_LINUX -DZENOH_TRANSPORT_UDP_IP -DZENOH_DEBUG=1)
set (ZENOH_TRANSPORT_UDP "ON")


set(PICO /home/lieven/workspace/zenoh-pico)
set(PICO_SRCS ${PICO}/src/codec.c 
  ${PICO}/src/collection.c
  ${PICO}/src/iobuf.c
  ${PICO}/src/utils.c
  ${PICO}/src/net/codec.c
  ${PICO}/src/net/internal.c
  ${PICO}/src/net/lease_task.c
  ${PICO}/src/net/msgcodec.c
  ${PICO}/src/net/property.c
  ${PICO}/src/net/read_task.c
  ${PICO}/src/net/rname.c
  ${PICO}/src/net/session.c
  ${PICO}/src/net/unix/network.c
  ${PICO}/src/net/unix/system.c
  ${PICO}/src/compat/strdup.c
 )


include_directories(
  src 
  ${PICO}/include
../limero/inc 
../limero/linux  
/home/lieven/workspace/ArduinoJson/src )

add_definitions(-DCATCH_CONFIG_DISABLE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -DZENOH_LINUX -DZENOH_TRANSPORT_UDP_IP -DZENOH_DEBUG=1")
set (CMAKE_CXX_STANDARD 11)

 
# set up our program
add_executable(loopback)
add_executable(proxy)


# declare our source files
target_sources(loopback PRIVATE src/loopback.cpp  src/serial.cpp  ${PICO_SRCS} src/config.cpp )
target_sources(proxy PRIVATE src/proxy.cpp src/ppp_frame.cpp src/Udp.cpp src/serial.cpp src/config.cpp ) 


target_link_libraries(loopback 
-L/home/lieven/workspace/limero/linux/build -llimero 
-lpthread  
-lrt  
-latomic)

target_link_libraries(proxy 
-L/home/lieven/workspace/limero/linux/build -llimero 
-lpthread  
-lrt  
-latomic)



