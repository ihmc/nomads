## Android makefile
## Run ./build_TinyXPath_android in order to build this module with dependecies
## written by Enrico Casini

LOCAL_PATH := $(call my-dir)/../..

include $(CLEAR_VARS)

LOCAL_SRC_FILES := action_store.cpp \
    htmlutil.cpp \
    lex_util.cpp \
    node_set.cpp \
    tinystr.cpp \
    tinyxml.cpp \
    tinyxmlerror.cpp \
    tinyxmlparser.cpp \
    tokenlist.cpp \
    xml_util.cpp \
    xpath_expression.cpp \
    xpath_processor.cpp \
    xpath_stack.cpp \
    xpath_static.cpp \
    xpath_stream.cpp \
    xpath_syntax.cpp \
    main.cpp
    
LOCAL_MODULE    := tinyxpath
LOCAL_CPPFLAGS	:= -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM
LOCAL_LDLIBS := -lz -ldl

#LOCAL_C_INCLUDES += \
#	$(LOCAL_PATH)/../../../util/cpp
	
#LOCAL_SHARED_LIBRARIES := \

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)
