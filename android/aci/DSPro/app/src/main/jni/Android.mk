## Android makefile
## written by Enrico Casini

LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)
LOCAL_MODULE := util
LOCAL_SRC_FILES := ../obj/local/armeabi/libutil.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../util/cpp
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := sqlite3droid
LOCAL_SRC_FILES := ../obj/local/armeabi/libsqlite3droid.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../externals/SQLite
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libjpegdroid
LOCAL_SRC_FILES := ../obj/local/armeabi/libjpegdroid.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../externals/libjpeg
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := tinyxpath
LOCAL_SRC_FILES := ../obj/local/armeabi/libtinyxpath.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../externals/TinyXPath
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := msgpack
LOCAL_SRC_FILES := ../obj/local/armeabi/libmsgpack.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../externals/msgpack
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := cnats
LOCAL_SRC_FILES := ../obj/local/armeabi/libcnats.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../externals/cnats/cnats-master
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := lcppdc
LOCAL_SRC_FILES := ../obj/local/armeabi/liblcppdc.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../misc/cpp/lcppdc
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := voi
LOCAL_SRC_FILES := ../obj/local/armeabi/libvoi.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../misc/cpp/voi
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := c4.5
LOCAL_SRC_FILES := ../obj/local/armeabi/libc4.5.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../misc/cpp/c45
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := chunking
LOCAL_SRC_FILES := ../obj/local/armeabi/libchunking.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../misc/cpp/chunking
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := natswr
LOCAL_SRC_FILES := ../obj/local/armeabi/libnatswr.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../misc/cpp/natswr
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ihmcmedia
LOCAL_SRC_FILES := ../obj/local/armeabi/libihmcmedia.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../misc/cpp/media
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := dali
LOCAL_SRC_FILES := ../obj/local/armeabi/libdali.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../externals/dali-1.0
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := milstd2525 
LOCAL_SRC_FILES := ../obj/local/armeabi/libmilstd2525.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../misc/cpp/milstd2525
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := disservice
LOCAL_SRC_FILES := ../obj/local/armeabi/libdisservice.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../aci/cpp/DisService
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := mockets
LOCAL_SRC_FILES := ../obj/local/armeabi/libmockets.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../mockets/cpp/
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := nms 
LOCAL_SRC_FILES := ../obj/local/armeabi/libnms.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../nms/cpp/
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := dspro2
LOCAL_SRC_FILES := ../obj/local/armeabi/libdspro2.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../../../../../aci/cpp/dspro2
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := nockets
LOCAL_SRC_FILES := ../obj/local/armeabi/libnockets.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../misc/cpp/nockets
include $(PREBUILT_SHARED_LIBRARY)

#include $(CLEAR_VARS)
#LOCAL_MODULE := mil_navy_nrl_norm
#LOCAL_SRC_FILES := ../obj/local/armeabi/libmil_navy_nrl_norm.so
#LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../externals/norm-1.5r6/include
#include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := DSProAndroid.cpp \
DSProAndroidWrapper.cpp
LOCAL_MODULE    := dsproandroid
LOCAL_CPPFLAGS	:= -fexceptions -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM -DENABLE_MUTEX_LOGGING
LOCAL_CFLAGS := -fsigned-char
LOCAL_LDLIBS := -lz 

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../ \
	$(LOCAL_PATH)/../../../../../../../aci/cpp/dspro2 \
	$(LOCAL_PATH)/../../../../../../../aci/cpp/dspro2/ranking \
	$(LOCAL_PATH)/../../../../../../../aci/cpp/dspro2/awareness \
	$(LOCAL_PATH)/../../../../../../../aci/cpp/dspro2/comm \
	$(LOCAL_PATH)/../../../../../../../.aci/cpp/dspro2/comm/disservice \
	$(LOCAL_PATH)/../../../../../../../aci/cpp/dspro2/comm/mockets \
	$(LOCAL_PATH)/../../../../../../../aci/cpp/dspro2/proxy \
	$(LOCAL_PATH)/../../../../../../../aci/cpp/dspro2/scheduler \
	$(LOCAL_PATH)/../../../../../../../aci/cpp/dspro2/storage \
    	$(LOCAL_PATH)/../../../../../../../nms/cpp \
    	$(LOCAL_PATH)/../../../../../../../util/cpp \
    	$(LOCAL_PATH)/../../../../../../../util/cpp/net \
    	$(LOCAL_PATH)/../../../../../../../util/cpp/graph \
    	$(LOCAL_PATH)/../../../../../../../externals/SQLite \
    	$(LOCAL_PATH)/../../../../../../../externals/libjpeg \
    	$(LOCAL_PATH)/../../../../../../../externals/TinyXPath \
    	$(LOCAL_PATH)/../../../../../../../misc/cpp/c45 \
    	$(LOCAL_PATH)/../../../../../../../misc/cpp/lcppdc \
    	$(LOCAL_PATH)/../../../../../../../misc/cpp/chunking \
    	$(LOCAL_PATH)/../../../../../../../misc/cpp/chunking/proxy \
	$(LOCAL_PATH)/../../../../../../../misc/cpp/media \
    	$(LOCAL_PATH)/../../../../../../../misc/cpp/milstd2525 \
	$(LOCAL_PATH)/../../../../../../../misc/cpp/voi \
        $(LOCAL_PATH)/../../../../../../../misc/cpp/voi/core \
        $(LOCAL_PATH)/../../../../../../../misc/cpp/voi/ctxt \
        $(LOCAL_PATH)/../../../../../../../misc/cpp/voi/util \
	$(LOCAL_PATH)/../../../../../../../mockets/cpp \
    	$(LOCAL_PATH)/../../../../../../../aci/cpp/DisService

LOCAL_SHARED_LIBRARIES := disservice \
    dspro2 \
    mockets \
    nockets \
    sqlite3droid \
    libjpegdroid \
    chunking \
    voi \
    ihmcmedia \
    tinyxpath \
    msgpack \
    lcppdc \
    c4.5 \
    milstd2525 \
    nms \
    dali \
    util

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)
