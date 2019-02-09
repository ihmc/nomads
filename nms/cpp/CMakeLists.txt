cmake_minimum_required(VERSION 3.10)
project(nms)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (UNIX)
    add_definitions(-DUNIX)
    add_definitions(-DLITTLE_ENDIAN_SYSTEM)
    if("${CMAKE_SYSTEM}" MATCHES "Linux")
        add_definitions(-DLINUX)
    endif()
else()
    add_definitions(-DWIN32)
endif (UNIX)

include_directories(. ifaces proxy proxy/client proxy/server)

#add_subdirectory(../../util/cpp util)
include_directories(../../util/cpp ../../util/cpp/net ../../util/cpp/proxy ../../util/cpp/security)

add_subdirectory(../../misc/cpp/nockets nockets)
include_directories(../../misc/cpp/nockets)

set(CMAKE_CXX_FLAGS " -pthread ")

add_library(nms
        Fragmenter.cpp
	Fragmenter.h
	ifaces/AbstractNetworkInterface.cpp
	ifaces/AbstractNetworkInterface.h
	ifaces/DatagramBasedAbstractNetworkInterface.cpp
	ifaces/DatagramBasedAbstractNetworkInterface.h
	ifaces/LocalNetworkInterface.cpp
	ifaces/LocalNetworkInterface.h
	ifaces/NormNetworkInterface.cpp
	ifaces/NormNetworkInterface.h
	ifaces/ProxyNetworkInterface.cpp
	ifaces/ProxyNetworkInterface.h
	ManycastForwardingNetworkInterface.cpp
	ManycastForwardingNetworkInterface.h
	ManycastNetworkMessageReceiver.cpp
	ManycastNetworkMessageReceiver.h
	MessageFactory.cpp
	MessageFactory.h
	NetworkInterface.cpp
	NetworkInterfaceFactory.cpp
	NetworkInterfaceFactory.h
	NetworkInterface.h
	NetworkInterfaceManager.cpp
	NetworkInterfaceManager.h
	NetworkMessage.cpp
	NetworkMessage.h
	NetworkMessageReceiver.cpp
	NetworkMessageReceiver.h
	NetworkMessageService.cpp
	NetworkMessageService.h
	NetworkMessageServiceImpl.cpp
	NetworkMessageServiceImpl.h
	NetworkMessageServiceInterface.h
	NetworkMessageServiceListener.h
	NetworkMessageV1.cpp
	NetworkMessageV1.h
	NetworkMessageV2.cpp
	NetworkMessageV2.h
	NMSProperties.cpp
	NMSProperties.h
	proxy/NetworkMessageServiceUnmarshaller.cpp
	proxy/NetworkMessageServiceUnmarshaller.h
	proxy/SerializationUtil.cpp
	proxy/SerializationUtil.h
	Reassembler.cpp
	Reassembler.h
	TSNRangeHandler.cpp
	TSNRangeHandler.h)

add_executable(NMSLauncher NMSLauncher.cpp)
target_link_libraries(NMSLauncher nms nockets util)
