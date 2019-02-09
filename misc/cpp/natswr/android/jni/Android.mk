## Android makefile
## Run 'make' in order to build this module with dependecies
## written by Giacomo Benincasa

LOCAL_PATH := $(call my-dir)/../..

include $(CLEAR_VARS)
LOCAL_MODULE := util
LOCAL_SRC_FILES := android/obj/local/armeabi/libutil.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../util/cpp
include $(PREBUILT_SHARED_LIBRARY)

#include $(CLEAR_VARS)
#LOCAL_MODULE := protobuf
#LOCAL_SRC_FILES := $(LOCAL_PATH)../../../android/externals/protobuf/prebuilt/libprotobuf.a
#LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../android/externals/protobuf/prebuilt/include
#include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := cnats
LOCAL_SRC_FILES := android/obj/local/armeabi/libcnats.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../externals/cnats/cnats-master/src
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := NatsWrapper.cpp
LOCAL_MODULE := natswr
LOCAL_CPPFLAGS:= -fexceptions -frtti -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM -std=c++11
LOCAL_CFLAGS := -fsigned-char
LOCAL_LDLIBS := -lz

TARGET_PLATFORM := android-21

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/ \
        $(LOCAL_PATH)/../../util \
	$(LOCAL_PATH)/../../externals/cnats/cnats-master/src \
	$(LOCAL_PATH)/../../android/externals/protobuf/prebuilt/include	

LOCAL_SHARED_LIBRARIES := util cnats

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)

