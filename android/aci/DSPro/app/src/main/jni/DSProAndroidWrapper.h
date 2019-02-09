#include "jni.h"

extern "C" {
    JNIEXPORT jint JNICALL Java_us_ihmc_android_aci_dspro_DSProService_startDSPro(
			   JNIEnv * env, jobject obj, jint ui16Port, jstring storageDir, jstring version);
}

extern "C" {
    JNIEXPORT jint JNICALL Java_us_ihmc_android_aci_dspro_DSProService_startDisService(
			   JNIEnv * env, jobject obj, jint ui16Port, jstring storageDir, jstring version);
}

extern "C" {
    JNIEXPORT jint JNICALL Java_us_ihmc_android_aci_dspro_DSProService_reloadTransmissionService(
			   JNIEnv * env, jobject obj);
}

extern "C" {
    JNIEXPORT jint JNICALL Java_us_ihmc_android_aci_dspro_DSProService_reloadCommAdaptors(
			   JNIEnv * env, jobject obj);
}

extern "C" {
    JNIEXPORT jint JNICALL Java_us_ihmc_android_aci_dspro_DSProService_stopDSPro(
			   JNIEnv * env, jobject obj);
}
