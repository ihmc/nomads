## Android makefile
## Run 'make' in order to build this module with dependecies
## written by Enrico Casini

LOCAL_PATH := $(call my-dir)/../..

include $(CLEAR_VARS)
LOCAL_SRC_FILES := png.c \
pngerror.c \
pngget.c \
pngmem.c \
pngpread.c \
pngread.c \
pngrio.c \
pngrtran.c \
pngrutil.c \
pngset.c \
pngtest.c \
pngtrans.c \
pngwio.c \
pngwrite.c \
pngwtran.c \
pngwutil.c
    
LOCAL_MODULE    := libpng
LOCAL_CPPFLAGS	:= -fexceptions -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM
LOCAL_LDLIBS := -lz -ldl
#LOCAL_SHARED_LIBRARIES := \

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)

