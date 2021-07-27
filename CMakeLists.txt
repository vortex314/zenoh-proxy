cmake_minimum_required(VERSION 3.12)
project(spine)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 11)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_definitions(-DZENOH_LINUX -DZENOH_TRANSPORT_UDP_IP -DZENOH_DEBUG=1 -DLINUX)
set (ZENOH_TRANSPORT_UDP "ON")


set(PICO /home/lieven/workspace/zenoh-pico)
set(ZENOHC /home/lieven/workspace/zenoh-c)

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
#  ${PICO}/src/net/unix/network.c
  ${PICO}/src/net/unix/system.c
  ${PICO}/src/compat/strdup.c
 )


include_directories(
  src 
  ${ZENOHC}/include
../limero/inc 
../limero/linux  
  common
../tinycbor/src
/home/lieven/workspace/ArduinoJson/src )

add_definitions(-DCATCH_CONFIG_DISABLE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -DZENOH_LINUX -DZENOH_TRANSPORT_UDP_IP -DZENOH_DEBUG=1")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -g -DZENOH_LINUX -DZENOH_TRANSPORT_UDP_IP -DZENOH_DEBUG=1")
# set(CMAKE_BUILD_TYPE Debug)

set (CMAKE_CXX_STANDARD 11)

 
# set up our program
add_executable(proxy)
add_executable(test)

target_sources(test PRIVATE 
  src/test.cpp 
  src/util.cpp
  common/CborSerializer.cpp
  common/CborDeserializer.cpp
  common/CborDump.cpp
 ) 

target_sources(proxy PRIVATE 
  src/proxy.cpp 
  common/ppp_frame.cpp 
  src/serial.cpp 
  src/config.cpp 
  src/util.cpp
  src/serial_session.cpp
  src/BrokerZenoh.cpp
  common/CborSerializer.cpp
  common/CborDeserializer.cpp
  common/CborDump.cpp
  common/Frame.cpp) 

target_link_libraries(proxy 
-L/home/lieven/workspace/limero/linux/build -llimero 
-L/home/lieven/workspace/tinycbor/lib -l:libtinycbor.a
-L/usr/local/lib/ -lzenohc
-lpthread  
-lrt  
-latomic)

target_link_libraries(test 
-L/home/lieven/workspace/limero/linux/build -llimero 
-L/home/lieven/workspace/tinycbor/lib -l:libtinycbor.a
gtest_main
-lpthread  
-lrt  
-latomic)

include(GoogleTest)
gtest_discover_tests(test)



