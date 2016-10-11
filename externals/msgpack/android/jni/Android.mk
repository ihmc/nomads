## Android makefile
## Run ./build_TinyXPath_android in order to build this module with dependecies
## written by Giacomo Benincasa

LOCAL_PATH := $(call my-dir)/../../src

include $(CLEAR_VARS)

LOCAL_SRC_FILES := gcc_atomic.cpp \
	object.cpp \
	../../../util/cpp/EndianHelper.cpp
    
LOCAL_MODULE    := msgpack
LOCAL_CPPFLAGS	:= -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM -fexceptions
LOCAL_LDLIBS := -lz -ldl

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../../util/cpp
	
#LOCAL_SHARED_LIBRARIES := \

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)
