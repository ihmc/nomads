# Android makefile
## Random message
## written by Roberto Fronteddu
# export PATH=$PATH:/home/nomads/Desktop/folder/android-ndk-r11c/   + ndk-build from the jni folder
LOCAL_PATH := $(call my-dir)/../..

	#List of dependencies
#util
include $(CLEAR_VARS)
LOCAL_MODULE := util
LOCAL_SRC_FILES := ../../../util/cpp/android/libs/armeabi/libutil.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../util/cpp
include $(PREBUILT_SHARED_LIBRARY)

#protobuf
include $(CLEAR_VARS)
LOCAL_MODULE := protobuf
LOCAL_SRC_FILES := android/prebuilt/libprotobuf.a
LOCAL_EXPORT_C_INCLUDES :=  $(LOCAL_PATH)/android/include
include $(PREBUILT_STATIC_LIBRARY)

#libpcap
include $(CLEAR_VARS)
LOCAL_MODULE := pcap
LOCAL_SRC_FILES := android/prebuilt/libpcap.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/android/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_CPPFLAGS := -std=c++11
LOCAL_CFLAGS := -D GOOGLE_PROTOBUF_NO_RTTI=1 -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM
LOCAL_SRC_FILES := \
	NetSensor.cpp \
	NetSensorLauncher.cpp \
	NetworkInterface.cpp \
	PCapInterface.cpp \
	topology.pb.cc \
	traffic.pb.cc
LOCAL_MODULE := netsensor
LOCAL_LDLIBS := -lz -lc


## netSensor.cpp
#LOCAL_MODULE    := netSensor
#LOCAL_CPPFLAGS	:= -fexceptions -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM -DENABLE_MUTEX_LOGGING -frtti
#LOCAL_CFLAGS := -fsigned-char
#LOCAL_LDLIBS := -lz -ldl -llog -lm -lc

#files
LOCAL_CPP_EXTENSION := .cpp .cc .c
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../../util/cpp \
   	$(LOCAL_PATH)/../../../util/cpp/net

LOCAL_SHARED_LIBRARIES := util

LOCAL_STATIC_LIBRARIES := \
	protobuf \
	pcap

#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_EXECUTABLE)



