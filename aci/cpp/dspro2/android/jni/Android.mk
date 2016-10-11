:q# Android makefile
## Run 'make' in order to build this module with dependecies
## written by Enrico Casini

LOCAL_PATH := $(call my-dir)/../..

# List of dependencies

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
LOCAL_MODULE := dali
LOCAL_SRC_FILES := android/obj/local/armeabi/libdali.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../../externals/dali-1.0
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := nms
LOCAL_SRC_FILES := android/obj/local/armeabi/libnms.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../nms/cpp
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := sqlite3droid
LOCAL_SRC_FILES := android/obj/local/armeabi/libsqlite3droid.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../externals/SQLite
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libjpegdroid
LOCAL_SRC_FILES := android/obj/local/armeabi/libjpegdroid.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../externals/libjpeg
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := tinyxpath
LOCAL_SRC_FILES := android/obj/local/armeabi/libtinyxpath.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../externals/TinyXPath
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := msgpack 
LOCAL_SRC_FILES := android/obj/local/armeabi/libmsgpack.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../externals/msgpack
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := lcppdc
LOCAL_SRC_FILES := android/obj/local/armeabi/liblcppdc.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../misc/cpp/lcppdc
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := c4.5
LOCAL_SRC_FILES := android/obj/local/armeabi/libc4.5.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../misc/cpp/c45
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := chunking
LOCAL_SRC_FILES := android/obj/local/armeabi/libchunking.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../misc/cpp/chunking
include $(PREBUILT_SHARED_LIBRARY)

#include $(CLEAR_VARS)
#LOCAL_MODULE := nockets
#LOCAL_SRC_FILES := android/obj/local/armeabi/libnockets.so
#LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../misc/cpp/nockets
#include $(PREBUILT_SHARED_LIBRARY)

#include $(CLEAR_VARS)
#LOCAL_MODULE := mil_navy_nrl_norm
#LOCAL_SRC_FILES := android/obj/local/armeabi/libmil_navy_nrl_norm.so
#LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../externals/norm-1.5r6/include
#include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := milstd2525
LOCAL_SRC_FILES := android/obj/local/armeabi/libmilstd2525.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../misc/cpp/milstd2525
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := mockets
LOCAL_SRC_FILES := android/obj/local/armeabi/libmockets.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../mockets/cpp/
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := disservice
LOCAL_SRC_FILES := android/obj/local/armeabi/libdisservice.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../DisService
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
ranking/BoundingBox.cpp \
ranking/CustomPolicies.cpp \
ranking/DSLib.cpp \
ranking/Match.cpp \
ranking/MatchmakingIntrumentation.cpp \
ranking/MatchmakingQualifier.cpp \
ranking/MatchMakingPolicies.cpp \
ranking/MetaData.cpp \
ranking/MetadataConfiguration.cpp \
ranking/MetadataInterface.cpp \
ranking/MetaDataRanker.cpp \
ranking/MetadataRankerConfiguration.cpp \
ranking/NodeContext.cpp \
ranking/NodeIdSet.cpp \
ranking/NodePath.cpp \
ranking/Path.cpp \
ranking/Pedigree.cpp \
ranking/RankByTargetMap.cpp \
ranking/Rank.cpp \
ranking/RankFactory.cpp \
ranking/SQLAVList.cpp \
awareness/C45LocalNodeContext.cpp \
awareness/LocalNodeContext.cpp \
awareness/NodeContextManager.cpp \
awareness/NonClassifyingLocalNodeContext.cpp \
awareness/PeerNodeContext.cpp \
awareness/PositionUpdater.cpp \
awareness/Targets.cpp \
awareness/Topology.cpp \
comm/AdaptorProperties.cpp \
comm/CommAdaptor.cpp \
comm/CommAdaptorListenerNotifier.cpp \
comm/ConnCommAdaptor.cpp \
comm/ConnHandler.cpp \
comm/ConnListener.cpp \
comm/SearchProperties.cpp \
comm/SessionIdChecker.cpp \
comm/disservice/DisServiceAdaptor.cpp \
comm/disservice/DSProMessage.cpp \
comm/mockets/MocketConnHandler.cpp \
comm/mockets/MocketConnListener.cpp \
comm/mockets/MocketsAdaptor.cpp \
comm/mockets/MocketsEndPoint.cpp \
comm/mockets/TagGenerator.cpp \
comm/tcp/TCPAdaptor.cpp \
comm/tcp/TCPConnHandler.cpp \
comm/tcp/TCPConnListener.cpp \
comm/tcp/TCPEndPoint.cpp \
controllers/forwarding/MessageForwardingController.cpp \
controllers/query/ApplicationQueryController.cpp \
controllers/query/ChunkQuery.cpp \
controllers/query/ChunkQueryController.cpp \
controllers/query/DisServiceQueryController.cpp \
controllers/query/DSProQueryController.cpp \
controllers/query/QueryController.cpp \
controllers/query/QueryQualifierBuilder.cpp \
proxy/DSProProxy.cpp \
proxy/DSProProxyAdaptor.cpp \
proxy/DSProProxyServer.cpp \
scheduler/MessageRequestServer.cpp \
scheduler/Scheduler.cpp \
scheduler/SchedulerCache.cpp \
scheduler/SchedulerGeneratedMetadata.cpp \
scheduler/SchedulerPolicies.cpp \
storage/DataStore.cpp \
storage/InformationStore.cpp \
storage/Publisher.cpp \
storage/StorageController.cpp \
CallbackHandler.cpp \
CommAdaptorManager.cpp \
Controller.cpp \
ControlMessageNotifier.cpp \
DSPro.cpp \
DSProImpl.cpp \
DSProListener.cpp \
DSProServices.cpp \
DSProUtils.cpp \
InformationPull.cpp \
InformationPush.cpp \
InformationPushPolicy.cpp \
Instrumentator.cpp \
MatchmakingHelper.cpp \
MessageForwardingPolicy.cpp \
MessageHeaders.cpp \
MessageIdGenerator.cpp \
MetadataGenerator.cpp \
UserRequests.cpp \
WaypointMessageHelper.cpp

LOCAL_MODULE    := dspro2
LOCAL_CPPFLAGS	:= -fexceptions -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM -DUSE_SQLITE -DENABLE_MUTEX_LOGGING
LOCAL_CFLAGS := -fsigned-char
LOCAL_LDLIBS := -lz -ldl

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/ranking \
	$(LOCAL_PATH)/awareness \
	$(LOCAL_PATH)/comm \
	$(LOCAL_PATH)/comm/disservice \
	$(LOCAL_PATH)/comm/mockets \
	$(LOCAL_PATH)/comm/tcp \
	$(LOCAL_PATH)/controllers/query \
	$(LOCAL_PATH)/controllers/forwarding \
	$(LOCAL_PATH)/proxy \
	$(LOCAL_PATH)/scheduler \
	$(LOCAL_PATH)/storage \
	$(LOCAL_PATH)/../../../util/cpp \
	$(LOCAL_PATH)/../../../util/cpp/net \
	$(LOCAL_PATH)/../../../util/cpp/graph \
	$(LOCAL_PATH)/../../../externals/SQLite \
	$(LOCAL_PATH)/../../../externals/libjpeg \
	$(LOCAL_PATH)/../../../externals/TinyXPath \
	$(LOCAL_PATH)/../../../externals/msgpack/include \
	$(LOCAL_PATH)/../../../misc/cpp/c45 \
	$(LOCAL_PATH)/../../../misc/cpp/lcppdc \
	$(LOCAL_PATH)/../../../misc/cpp/media \
	$(LOCAL_PATH)/../../../misc/cpp/chunking \
	$(LOCAL_PATH)/../../../misc/cpp/milstd2525 \
	$(LOCAL_PATH)/../../../mockets/cpp \
	$(LOCAL_PATH)/../DisService
#	$(LOCAL_PATH)/../../android/externals/openssl/jni/include \
#	$(LOCAL_PATH)/../../android/externals/openssl/jni/crypto

LOCAL_SHARED_LIBRARIES := disservice \
	mockets \
    sqlite3droid \
    libjpegdroid \
    chunking \
    milstd2525 \
    tinyxpath \
    msgpack \
    ihmcmedia \
    lcppdc \
    c4.5 \
    nms \
    dali \
    util
#    nocket \
#    mil_navy_nrl_norm \

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)
