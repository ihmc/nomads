## Android makefile
## Run 'make' in order to build this module with dependecies
## written by Enrico Casini

LOCAL_PATH := $(call my-dir)/../..

include $(CLEAR_VARS)
LOCAL_SRC_FILES := cdjpeg.c \
jaricom.c \
jcapimin.c \
jcapistd.c \
jcarith.c \
jccoefct.c \
jccolor.c \
jcdctmgr.c \
jchuff.c \
jcinit.c \
jcmainct.c \
jcmarker.c \
jcmaster.c \
jcomapi.c \
jcparam.c \
jcprepct.c \
jcsample.c \
jctrans.c \
jdapimin.c \
jdapistd.c \
jdarith.c \
jdatadst.c \
jdatasrc.c \
jdcoefct.c \
jdcolor.c \
jddctmgr.c \
jdhuff.c \
jdinput.c \
jdmainct.c \
jdmarker.c \
jdmaster.c \
jdmerge.c \
jdpostct.c \
jdsample.c \
jdtrans.c \
jerror.c \
jfdctflt.c \
jfdctfst.c \
jfdctint.c \
jidctflt.c \
jidctfst.c \
jidctint.c \
jmemmgr.c \
jmem-android.c \
jpegtran.c \
jquant1.c \
jquant2.c \
jutils.c \
rdbmp.c \
rdcolmap.c \
rdgif.c \
rdppm.c \
rdrle.c \
rdswitch.c \
rdtarga.c \
transupp.c \
wrbmp.c \
wrgif.c \
wrppm.c \
wrrle.c \
wrtarga.c
    
LOCAL_MODULE    := libjpegdroid
LOCAL_CPPFLAGS	:= -fexceptions -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM
LOCAL_LDLIBS := -lz -ldl
#LOCAL_SHARED_LIBRARIES := \

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)

