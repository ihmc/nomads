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
#LOCAL_MODULE := mil_navy_nrl_norm
#LOCAL_SRC_FILES := android/obj/local/armeabi/libmil_navy_nrl_norm.so
#LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../externals/norm-1.5r6/include
#include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := Nocket.cpp \
    NormUtil.cpp   

LOCAL_MODULE    := nockets
LOCAL_CPPFLAGS	:= -fexceptions -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM
LOCAL_CFLAGS := -fsigned-char
LOCAL_LDLIBS := -lz -ldl
LOCAL_SHARED_LIBRARIES := util # mil_navy_nrl_norm

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/../../../util/cpp \
    $(LOCAL_PATH)/../../../util/cpp/net
    # $(LOCAL_PATH)/../../../externals/norm-1.5r6/include


include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)

