OUT_DIR=obj/local/armeabi

all: build

prebuild:
	make -C ../../DisService/android
	make -C ../../../../misc/cpp/c45/android
	make -C ../../../../misc/cpp/milstd2525/android
	make -C ../../../../mockets/cpp/jni/android
	if [ ! -d $(OUT_DIR) ]; then mkdir -p $(OUT_DIR); fi;
	cp -Rv ../../DisService/android/libs/armeabi/*.so $(OUT_DIR)
	cp -Rv ../../../../misc/cpp/c45/android/libs/armeabi/*.so $(OUT_DIR)
	cp -Rv ../../../../misc/cpp/milstd2525/android/libs/armeabi/*.so $(OUT_DIR)
	cp -Rv ../../../../mockets/cpp/jni/android/libs/armeabi/*.so $(OUT_DIR)
	
build: prebuild
	ndk-build

clean:
	rm -rf libs obj

cleanall: clean
	make -C ../../DisService/android cleanall
	make -C ../../../../misc/cpp/c45/android clean
	make -C ../../../../misc/cpp/milstd2525/android clean
	make -C ../../../../mockets/cpp/android clean
