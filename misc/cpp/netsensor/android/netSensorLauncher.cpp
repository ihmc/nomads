#include "us_ihmc_android_misc_netsensor_MainActivity.h"
#include <jni.h>
#include "NetSensor.h"

JNIEXPORT jint JNICALL Java_us_ihmc_android_misc_netsensor_MainActivity_LaunchNetSensor
  (JNIEnv *, jobject) 
{
    	IHMC_MISC::NetSensor theSensor;
	int rc;
        if (0 != (rc = theSensor.init("/storage/emulated/0/netSensor.cfg")))
		return rc;
	return 0;
}


