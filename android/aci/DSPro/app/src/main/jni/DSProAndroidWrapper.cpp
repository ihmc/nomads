#include "DSProAndroidWrapper.h"
#include "DSProAndroid.h"
#include "jni.h"

JNIEXPORT jint JNICALL Java_us_ihmc_android_aci_dspro_DSProService_startDSPro(
			   JNIEnv * env, jobject obj, jint ui16Port, jstring storageDir,
                           jstring version)
{	
    const char *pszStorageDir = env->GetStringUTFChars(storageDir, 0);
    const char *pszVersion = env->GetStringUTFChars(version, 0);
	return startDSPro(ui16Port, pszStorageDir, pszVersion);
    env->ReleaseStringUTFChars(storageDir, pszStorageDir);
    env->ReleaseStringUTFChars(version, pszVersion);
}

JNIEXPORT jint JNICALL Java_us_ihmc_android_aci_dspro_DSProService_startDisService(
			   JNIEnv * env, jobject obj, jint ui16Port, jstring storageDir,
                           jstring version)
{
    const char *pszStorageDir = env->GetStringUTFChars(storageDir, 0);
    const char *pszVersion = env->GetStringUTFChars(version, 0);
	return startDisService(ui16Port, pszStorageDir, pszVersion);
    env->ReleaseStringUTFChars(storageDir, pszStorageDir);
    env->ReleaseStringUTFChars(version, pszVersion);
}

JNIEXPORT jint JNICALL Java_us_ihmc_android_aci_dspro_DSProService_reloadTransmissionService(
			   JNIEnv * env, jobject obj)
{
	return reloadTransmissionService();
}

JNIEXPORT jint JNICALL Java_us_ihmc_android_aci_dspro_DSProService_reloadCommAdaptors(
			   JNIEnv * env, jobject obj)
{
	return reloadCommAdaptors();
}
    
JNIEXPORT jint JNICALL Java_us_ihmc_android_aci_dspro_DSProService_stopDSPro(
			   JNIEnv * env, jobject obj)
{	
	return stopDisServicePro();
}
