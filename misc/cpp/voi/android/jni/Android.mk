## Android makefile
## Run 'make' in order to build this module with dependecies
## written by Giacomo Benincasa

LOCAL_PATH := $(call my-dir)/../..

include $(CLEAR_VARS)
LOCAL_MODULE := util
LOCAL_SRC_FILES := android/obj/local/armeabi/libutil.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../util/cpp
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := milstd2525
LOCAL_SRC_FILES := android/obj/local/armeabi/libmilstd2525.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../cpp/milstd2525
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := lcppdc
LOCAL_SRC_FILES := android/obj/local/armeabi/liblcppdc.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../lcppdc
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := c4.5
LOCAL_SRC_FILES := android/obj/local/armeabi/libc4.5.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../c45
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := tinyxpath
LOCAL_SRC_FILES := android/obj/local/armeabi/libtinyxpath.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../externals/TinyXPath
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := core/Cache.cpp \
core/Comparator.cpp \
core/Match.cpp \
core/MatchMakingFilters.cpp \
core/MatchMakingPolicies.cpp \
core/MetadataImpl.cpp \
core/VoiImpl.cpp \
ctxt/AreaOfInterest.cpp \
ctxt/Path.cpp \
ctxt/Pedigree.cpp \
util/BoundingBox.cpp \
util/C45Utils.cpp \
util/NodeIdSet.cpp \
util/RankByTargetMap.cpp \
util/RankFactory.cpp \
InformationObject.cpp \
MetadataInterface.cpp \
MetadataRankerConfiguration.cpp \
MetaDataRanker.cpp \
MetadataRankerLocalConfiguration.cpp \
NodeContext.cpp \
NodePath.cpp \
Rank.cpp \
Score.cpp \
Voi.cpp \
VoiLauncher.cpp
    
LOCAL_MODULE    := voi
LOCAL_CPPFLAGS	:= -fexceptions -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM -std=c++11
LOCAL_CFLAGS := -fsigned-char
LOCAL_LDLIBS := -lz -ldl

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/ \
        $(LOCAL_PATH)/core \
        $(LOCAL_PATH)/ctxt \
        $(LOCAL_PATH)/util \
        $(LOCAL_PATH)/../c45 \
        $(LOCAL_PATH)/../milstd2525 \
        $(LOCAL_PATH)/../lcppdc \
        $(LOCAL_PATH)/../../externals/TinyXPath \
        $(LOCAL_PATH)/../../util
	

LOCAL_SHARED_LIBRARIES := util c4.5 lcppdc milstd2525 tinyxpath

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)

