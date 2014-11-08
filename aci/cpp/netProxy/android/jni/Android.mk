# Android makefile
## Run 'make' in order to build this module with dependecies
## written by Enrico Casini

LOCAL_PATH := $(call my-dir)/../..

# List of dependencies

include $(CLEAR_VARS)
LOCAL_MODULE := util
LOCAL_SRC_FILES := android/obj/local/armeabi/libutil.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../util/cpp
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := msgpack 
LOCAL_SRC_FILES := android/obj/local/armeabi/libmsgpack.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../externals/msgpack
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := mockets
LOCAL_SRC_FILES := android/obj/local/armeabi/libmockets.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../mockets/cpp/
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
ActiveConnection.cpp \
AutoConnectEntry.cpp \
CircularOrderedBuffer.cpp \
CompressionSetting.cpp \
Connection.cpp \
ConnectorAdapter.cpp \
Connector.cpp \
ConnectorReader.cpp \
ConnectorWriter.cpp \
Entry.cpp \
GUIUpdateMessage.cpp \
MocketConnector.cpp \
MutexBuffer.cpp \
MutexUDPQueue.cpp \
NetProxyConfigManager.cpp \
PacketRouter.cpp \
ProtocolSetting.cpp \
ProxyMessages.cpp \
ProxyNetworkMessage.cpp \
SocketConnector.cpp \
TapInterface.cpp \
TCPConnTable.cpp \
TCPManager.cpp \
TCPSegment.cpp \
TCPSocketAdapter.cpp \
UDPConnector.cpp \
UDPDatagramPacket.cpp \
UDPSocketAdapter.cpp \
Utilities.cpp \
ZLibConnectorReader.cpp \
ZLibConnectorWriter.cpp
#main.cpp \
#LzmaConnectorReader.cpp \
#LzmaConnectorWriter.cpp \


LOCAL_MODULE    := netproxy
LOCAL_CPPFLAGS	:= -fexceptions -frtti -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM
LOCAL_CFLAGS := -fsigned-char
LOCAL_LDLIBS := -lz -ldl

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/ \
	$(LOCAL_PATH)/../../../util/cpp \
	$(LOCAL_PATH)/../../../util/cpp/net \
	$(LOCAL_PATH)/../../../util/cpp/graph \
	$(LOCAL_PATH)/../../../externals/msgpack/include \
	$(LOCAL_PATH)/../../../externals/include \
	$(LOCAL_PATH)/../../../externals/include/pcap \
	$(LOCAL_PATH)/../../../mockets/cpp \
#	$(LOCAL_PATH)/../../android/externals/openssl/jni/include \
#	$(LOCAL_PATH)/../../android/externals/openssl/jni/crypto

LOCAL_SHARED_LIBRARIES := mockets \
    msgpack \
    util

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)
