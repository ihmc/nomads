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
LOCAL_SRC_FILES := C45AVList.cpp \
C45DecisionTree.cpp \
C45Rules.cpp \
C45RuleSetInfo.cpp \
C45RuleSetTestInfo.cpp \
C45RulesPrediction.cpp \
C45RulesTest.cpp \
C45TreePrediction.cpp \
C45TreeTestInfo.cpp \
Classifier.cpp \
DataGenerator.cpp \
DummyClassifier.cpp \
Prediction.cpp \
RandomClassifier.cpp \
RandomTestInfo.cpp \
besttree.c \
build.c \
c4.5.c \
c4.5rules.c \
classify.c \
confmat.c \
consult.c \
consultr.c \
contin.c \
discr.c \
genlogs.c \
genrules.c \
getdata.c \
getnames.c \
info.c \
makerules.c \
prune.c \
prunerule.c \
rules.c \
siftrules.c \
sort.c \
st-thresh.c \
stats.c \
subset.c \
testrules.c \
trees.c \
userint.c
    
LOCAL_MODULE    := c4.5
LOCAL_CPPFLAGS	:= -fexceptions -DUNIX -DLINUX -DANDROID -DLITTLE_ENDIAN_SYSTEM
LOCAL_LDLIBS := -lz -ldl
LOCAL_SHARED_LIBRARIES := util

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)

