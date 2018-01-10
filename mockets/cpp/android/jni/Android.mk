## Android makefile
## written by Enrico Casini

LOCAL_PATH := $(call my-dir)/../..

include $(CLEAR_VARS)
LOCAL_MODULE := util
LOCAL_SRC_FILES := android/obj/local/armeabi/libutil.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../util/cpp
include $(PREBUILT_SHARED_LIBRARY)

#openssl include
include $(CLEAR_VARS)
LOCAL_MODULE := ssl
LOCAL_SRC_FILES := ../../android/externals/openssl/openssl-1.0.2h/libssl.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../android/externals/openssl/openssl-1.0.2h/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := crypto
LOCAL_SRC_FILES := ../../android/externals/openssl/openssl-1.0.2h/libcrypto.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../android/externals/openssl/openssl-1.0.2h/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := ACKManager.cpp \
	BandwidthEstimator.cpp \
	CancelledTSNManager.cpp \
	CongestionController.cpp \
	Dtls.cpp	\
	DTLSCommInterface.cpp	\
	CommInterface.cpp \
	DataBuffer.cpp \
	MessageSender.cpp \
	Mocket.cpp \
	MocketReader.cpp \
	MocketStatusMonitor.cpp \
	MocketStatusNotifier.cpp \
	MocketWriter.cpp \
	Packet.cpp \
	PacketProcessor.cpp \
	Receiver.cpp \
	ServerMocket.cpp \
	StateCookie.cpp \
	StateMachine.cpp \
	StreamMocket.cpp \
	StreamServerMocket.cpp \
	Transmitter.cpp \
	TransmissionRateModulation.cpp \
	TSNRangeHandler.cpp \
	UDPCommInterface.cpp
	
LOCAL_MODULE    := mockets
LOCAL_CPPFLAGS	:= -fexceptions -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM
LOCAL_CFLAGS := -fsigned-char
LOCAL_LDLIBS := -lz 

#LOCAL_C_INCLUDES += \
#	$(LOCAL_PATH)/../../util/cpp \
#	$(LOCAL_PATH)/../../android/externals/openssl/jni/include \
#	$(LOCAL_PATH)/../../android/externals/openssl/jni/crypto

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../android/externals/openssl/openssl-1.0.2h/include \
	$(LOCAL_PATH)/../../android/externals/openssl/openssl-1.0.2h/crypto


LOCAL_SHARED_LIBRARIES := \
	ssl \
	crypto \
	util

LOCAL_DISABLE_FORMAT_STRING_CHECKS := true

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)

