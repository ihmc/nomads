OUT_DIR=obj/local/armeabi

all: build

prebuild:
	make -C ../../../../externals/SQLite/android
	make -C ../../../../externals/TinyXPath/android
	make -C ../../../../externals/msgpack/android
	make -C ../../../../misc/cpp/lcppdc/android
	make -C ../../../../misc/cpp/media/android
	make -C ../../../../misc/cpp/chunking/android
	make -C ../../../../misc/cpp/nockets/android
	make -C ../../../../nms/cpp/android
	if [ ! -d $(OUT_DIR) ]; then mkdir -p $(OUT_DIR); fi;
	cp -Rv ../../../../externals/SQLite/android/libs/armeabi/*.so $(OUT_DIR)
	cp -Rv ../../../../externals/TinyXPath/android/libs/armeabi/*.so $(OUT_DIR)
	cp -Rv ../../../../externals/msgpack/android/libs/armeabi/*.so $(OUT_DIR)
	cp -Rv ../../../../misc/cpp/lcppdc/android/libs/armeabi/*.so $(OUT_DIR)
	cp -Rv ../../../../misc/cpp/media/android/libs/armeabi/*.so $(OUT_DIR)
	cp -Rv ../../../../misc/cpp/chunking/android/libs/armeabi/*.so $(OUT_DIR)
	cp -Rv ../../../../misc/cpp/nockets/android/libs/armeabi/*.so $(OUT_DIR)
	cp -Rv ../../../../nms/cpp/android/libs/armeabi/*.so $(OUT_DIR)
	
build: prebuild
	ndk-build

clean:
	rm -rf libs obj

cleanall: clean
	make -C ../../../../externals/SQLite/android clean
	make -C ../../../../externals/TinyXPath/android clean
	make -C ../../../../externals/msgpack/android clean
	make -C ../../../../misc/cpp/lcppdc/android clean
	make -C ../../../../misc/cpp/media/android clean
	make -C ../../../../misc/cpp/chunking/android cleanall
	make -C ../../../../misc/cpp/nockets/android cleanall
	make -C ../../../../nms/cpp/android cleanall
