## Android makefile
## Run ./build_SQLite_android in order to build this module with dependecies
## written by Enrico Casini

LOCAL_PATH := $(call my-dir)/../..

include $(CLEAR_VARS)

LOCAL_SRC_FILES := sqlite3.c
    
LOCAL_MODULE    := sqlite3droid
LOCAL_CPPFLAGS	:= -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM
LOCAL_LDLIBS := -lz -ldl

#LOCAL_C_INCLUDES += \
#	$(LOCAL_PATH)/../../../util/cpp
	
#LOCAL_SHARED_LIBRARIES := \

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)
