OUT_DIR=app/src/main/obj/local/armeabi
LIBS_DIR=app/libs

all: apk

prebuild:
	if [ ! -d $(OUT_DIR) ]; then mkdir -p $(OUT_DIR); fi;
	if [ ! -d $(LIBS_DIR) ]; then mkdir -p $(LIBS_DIR); fi;
	cp app/src/main/*.properties .
	make -C ../../../aci/cpp/dspro2/android
	cp -Rv ../../../aci/cpp/dspro2/android/libs/armeabi/*.so $(OUT_DIR)
	ant -f ../../../aci/build/build.xml aci-android 
	./gradlew -p ../../../chunking androidJar
	cp -Rv ../../../chunking/build/libs/chunking-android-1.0.jar $(LIBS_DIR)
	cp -Rv ../../../aci/lib/aci-android.jar $(LIBS_DIR)
	./gradlew -p ../../../nms jar
	cp -Rv ../../../nms/lib/libs/nms-1.0-SNAPSHOT.jar $(LIBS_DIR);

build: prebuild
	mkdir -p app/src/main/jniLibs
	mkdir -p app/src/main/jniLibs/armeabi
	ndk-build -C app/src/main
	cp -Rv $(OUT_DIR)/*.so app/src/main/jniLibs/armeabi
	cp -Rv $(OUT_DIR)/objs/dsproandroid/* app/src/main/jniLibs/

apk: build
	./gradlew build
	cp -Rv app/build/outputs/apk/debug/app-debug.apk DSPro-debug-`date '+%Y%m%d_%H%M%S'`.apk

install: 
	./gradlew installDebug
	adb push app/src/main/assets/metadataExtraAttributes.xml /sdcard/IHMC/conf/
	adb push app/src/main/assets/metadataExtraValues.xml /sdcard/IHMC/conf/

clean:
	rm -rf *.apk
	rm -rf $(OUT_DIR)
	rm -rf app/src/main/jniLibs/*
	rm -f app/libs/nms-1.0-SNAPSHOT.jar
	rm -f app/libs/aci-android.jar
	./gradlew clean

cleanall: clean
	make -C ../../../aci/cpp/dspro2/android cleanall
