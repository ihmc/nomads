## Android makefile
## Run 'make' in order to build this module with dependecies
## written by Enrico Casini

LOCAL_PATH := $(call my-dir)/../..

include $(CLEAR_VARS)
LOCAL_MODULE := util
LOCAL_SRC_FILES := android/obj/local/armeabi/libutil.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../util/cpp
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := dali
LOCAL_SRC_FILES := android/obj/local/armeabi/libdali.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../externals/dali-1.0/include 
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := BMPImage.cpp \
	MPEG1Parser.cpp \
	MPEGElements.cpp \
	PPMImage.cpp \
	RGBImage.cpp
 
LOCAL_MODULE := ihmcmedia
LOCAL_CPPFLAGS := -fexceptions -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM
LOCAL_CFLAGS := -fsigned-char
LOCAL_LDLIBS := -lz -ldl

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../../externals/dali-1.0/include \
	$(LOCAL_PATH)/../../../util/cpp \
	$(LOCAL_PATH)/../../../util/cpp/net \
	$(LOCAL_PATH)/../../../util/cpp/graph

LOCAL_SHARED_LIBRARIES := util \
	dali

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)
