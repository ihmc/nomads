## Android makefile
## Run ./build_TinyXPath_android in order to build this module with dependecies
## written by Giacomo Benincasa

LOCAL_PATH := $(call my-dir)/../..

include $(CLEAR_VARS)

LOCAL_SRC_FILES := packages/basic/audio.c \
 packages/basic/bit.c \
 packages/basic/bitmake.c \
 packages/basic/bitparser.c \
 packages/basic/bitscan.c \
 packages/basic/bitsetop.c \
 packages/basic/bitstream.c \
 packages/basic/bitstreamfio.c \
 packages/basic/bitutil.c \
 packages/basic/byte16.c \
 packages/basic/byte32.c \
 packages/basic/bytearith.c \
 packages/basic/byte.c \
 packages/basic/bytetosc.c \
 packages/basic/crop.c \
 packages/basic/filter.c \
 packages/basic/float.c \
 packages/basic/jrevdct.c \
 packages/basic/mfwddct.c \
 packages/basic/qtables.c \
 packages/basic/scarith.c \
 packages/basic/sc.c \
 packages/basic/sctobyte.c \
 packages/basic/vector.c \
 packages/mpeg/block.c\
 packages/mpeg/bsearch.c \
 packages/mpeg/huffencode.c \
 packages/mpeg/motionsearch.c \
 packages/mpeg/mpegaudiohdr.c \
 packages/mpeg/mpegaudiol1.c \
 packages/mpeg/mpegaudiol2.c \
 packages/mpeg/mpegaudiol3.c \
 packages/mpeg/mpegaudiosyndata.c \
 packages/mpeg/mpegencode.c \
 packages/mpeg/mpeggophdr.c \
 packages/mpeg/mpegmacroblockhdr.c \
 packages/mpeg/mpegpckhdr.c \
 packages/mpeg/mpegpic.c \
 packages/mpeg/mpegpichdr.c \
 packages/mpeg/mpegpkthdr.c \
 packages/mpeg/mpegseqhdr.c \
 packages/mpeg/mpegslicehdr.c \
 packages/mpeg/mpegsyshdr.c \
 packages/mpeg/mpegsystoc.c \
 packages/mpeg/mpegvideoindex.c \
 packages/mpeg/psearch.c \
 packages/mpeg/startcode.c \
 packages/mpeg/synthesis.c \
 packages/mpeg/tables.c


LOCAL_MODULE    := dali 
LOCAL_CPPFLAGS	:= -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM
LOCAL_LDLIBS := -lz -ldl

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/packages/basic \
	$(LOCAL_PATH)/packages/mpeg
	
#LOCAL_SHARED_LIBRARIES := \

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)
