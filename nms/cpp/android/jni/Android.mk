## Android makefile
## written by Enrico Casini

LOCAL_PATH := $(call my-dir)/../..

include $(CLEAR_VARS)
LOCAL_MODULE := util
LOCAL_SRC_FILES := android/obj/local/armeabi/libutil.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../util/cpp
include $(PREBUILT_SHARED_LIBRARY)

#include $(CLEAR_VARS)
#LOCAL_MODULE := nockets
#LOCAL_SRC_FILES := android/obj/local/armeabi/libnockets.so
#LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../misc/cpp/nockets
#include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := Fragmenter.cpp \
        ManycastForwardingNetworkInterface.cpp \
	ManycastNetworkMessageReceiver.cpp \
	MessageFactory.cpp \
	NetworkInterface.cpp \
        NetworkInterfaceFactory.cpp \
        NetworkInterfaceManager.cpp \
	NetworkMessage.cpp \
	NetworkMessageReceiver.cpp \
	NetworkMessageService.cpp \
        NetworkMessageServiceImpl.cpp \
	NetworkMessageV1.cpp \
	NetworkMessageV2.cpp \
	NMSProperties.cpp \
	Reassembler.cpp \
	TSNRangeHandler.cpp \
	ifaces/AbstractNetworkInterface.cpp \
	ifaces/DatagramBasedAbstractNetworkInterface.cpp \
	ifaces/LocalNetworkInterface.cpp \
	ifaces/ProxyNetworkInterface.cpp \
	proxy/NetworkMessageServiceUnmarshaller.cpp \
	proxy/SerializationUtil.cpp \
	proxy/client/NetworkMessageServiceProxy.cpp \
	proxy/server/NetworkMessageServiceCallbackManager.cpp \
	proxy/server/NetworkMessageServiceProxyServer.cpp \
	proxy/server/NMSCommandProcessor.cpp

LOCAL_MODULE    := nms
LOCAL_CPPFLAGS	:= -fexceptions -frtti -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM -DENABLE_MUTEX_LOGGING
LOCAL_CFLAGS := -fsigned-char
LOCAL_LDLIBS := -lz 

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/ \
	$(LOCAL_PATH)/ifaces \
	$(LOCAL_PATH)/proxy \
	$(LOCAL_PATH)/proxy/client \
	$(LOCAL_PATH)/proxy/server \
	$(LOCAL_PATH)/../../misc/cpp/nockets \
	$(LOCAL_PATH)/../../util/cpp \
	$(LOCAL_PATH)/../../util/cpp/net \
	$(LOCAL_PATH)/../../util/cpp/proxy
#	$(LOCAL_PATH)/../../android/externals/openssl/jni/include \
#	$(LOCAL_PATH)/../../android/externals/openssl/jni/crypto

LOCAL_SHARED_LIBRARIES := \
	util \
	ssl \
	crypto
#nockets 

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)
