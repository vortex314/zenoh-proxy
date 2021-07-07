cmake_minimum_required(VERSION 3.12)
project(spine)

include(GNUInstallDirs)

find_package(iceoryx_posh CONFIG REQUIRED)
find_package(iceoryx_hoofs CONFIG REQUIRED)

get_target_property(ICEORYX_CXX_STANDARD iceoryx_posh::iceoryx_posh CXX_STANDARD)
include(IceoryxPlatform)

include_directories(
  src 
../limero/inc 
../limero/linux  
../tinycbor/src  
../cbor11 )

add_definitions(-DCATCH_CONFIG_DISABLE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

 
# set up our program
add_executable(peer)
add_executable(main)
add_executable(proxy)


# declare our source files
target_sources(peer PRIVATE src/peer.cpp)
target_sources(proxy PRIVATE src/proxy.cpp src/ppp_frame.cpp ../cbor11/cbor11.cpp src/serial.cpp )
target_sources(main PRIVATE src/main.cpp
  src/ppp_frame.cpp 
  src/session_serial.cpp
  ../cbor11/cbor11.cpp 
  src/ice.cpp
  src/serial.cpp)
target_compile_options(peer PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})
# target_compile_options(ee PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})


set_target_properties(
    peer 
    proxy
    main
    PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
    POSITION_INDEPENDENT_CODE ON
)


# link with YARP libraries
target_link_libraries(peer iceoryx_posh::iceoryx_posh)



target_link_libraries(main 
iceoryx_posh::iceoryx_posh
-L/home/lieven/workspace/limero/linux/build -llimero 
-lpthread  
-lrt  
-latomic)

target_link_libraries(proxy 
-L/home/lieven/workspace/limero/linux/build -llimero 
-lpthread  
-lrt  
-latomic)



