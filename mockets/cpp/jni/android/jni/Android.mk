## Android makefile
## written by Enrico Casini

LOCAL_PATH := $(call my-dir)/../..

include $(CLEAR_VARS)
LOCAL_MODULE := util
LOCAL_SRC_FILES := android/obj/local/armeabi/libutil.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../util/cpp
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := mockets
LOCAL_SRC_FILES := android/obj/local/armeabi/libmockets.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := MessageSenderWrapper.cpp \
MocketInputStreamWrapper.cpp \
MocketOutputStreamWrapper.cpp \
MocketWrapper.cpp \
ParamsWrapper.cpp \
ServerMocketWrapper.cpp \
StreamMocketWrapper.cpp \
StreamServerMocketWrapper.cpp
LOCAL_MODULE    := mocketsjavawrapper
LOCAL_CPPFLAGS	:= -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM
LOCAL_LDLIBS := -lz 
#LOCAL_C_INCLUDES += \
#    	$(LOCAL_PATH)/../ \
#    	$(LOCAL_PATH)/../../../util/cpp
#	$(LOCAL_PATH)/../../../android/externals/openssl/jni/include \
#	$(LOCAL_PATH)/../../../android/externals/openssl/jni/crypto

LOCAL_SHARED_LIBRARIES := \
    util \
    mockets

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)

