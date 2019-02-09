# Android makefile
## written by rfronteddu@ihmc.us

# This path has to point to the cnats src directory
LOCAL_PATH := $(call my-dir)/../../src

include $(CLEAR_VARS)
# This may not be a requirement
TARGET_PLATFORM := android-21

# If anyone has time, change the static file listing with something
# that generate the files automatically example:
#  FILE_LIST_SRC := $(wildcard $(LOCAL_PATH)/*.c)
#  FILE_LIST_SRC_UNIX := $(wildcard $(LOCAL_PATH)/UNIX/*.c)
#  LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%) $(FILE_LIST_SRC_UNIX:$(LOCAL_PATH)/%=%)

LOCAL_SRC_FILES := \
	asynccb.c \
	buf.c \
	comsock.c \
	conn.c \
	hash.c \
	msg.c \
	nats.c \
	natstime.c \
	nuid.c \
	opts.c \
	parser.c \
	pub.c \
	srvpool.c \
	stats.c \
	status.c \
	sub.c \
	timer.c \
	url.c \
	util.c \
	unix/cond.c \
	unix/mutex.c \
	unix/sock.c \
	unix/thread.c
	
LOCAL_MODULE := cnats

# You don't need all these flags
LOCAL_CPPFLAGS	:= -fexceptions -frtti -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM

#include $(BUILD_STATIC_LIBRARY)
include $(BUILD_SHARED_LIBRARY)
