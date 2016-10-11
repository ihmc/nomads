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
LOCAL_MODULE := ihmcmedia
LOCAL_SRC_FILES := android/obj/local/armeabi/libihmcmedia.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../misc/cpp/media
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libjpegdroid
LOCAL_SRC_FILES := android/obj/local/armeabi/libjpegdroid.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../externals/libjpeg
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := BMPHandler.cpp \
    Chunker.cpp \
    ChunkingUtils.cpp \
    ChunkReassembler.cpp \
    ImageCodec.cpp \
    JPEGLibWrapper.cpp \
    MimeUtils.cpp \
    MPEG1Handler.cpp \
    VideoCodec.cpp
    
LOCAL_MODULE    := chunking
LOCAL_CPPFLAGS	:= -fexceptions -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM
LOCAL_CFLAGS := -fsigned-char
LOCAL_LDLIBS := -lz -ldl

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../../../util/cpp \
	$(LOCAL_PATH)/../../../externals/libjpeg \
    $(LOCAL_PATH)/../../../misc/cpp/media

LOCAL_SHARED_LIBRARIES := util \
    ihmcmedia \
    libjpegdroid

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)

