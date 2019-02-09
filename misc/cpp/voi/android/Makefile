OUT_DIR=obj/local/armeabi

all: build

prebuild:
	make -C ../../../../util/cpp/android
	make -C ../../lcppdc/android
	make -C ../../milstd2525/android
	make -C ../../c45/android
	make -C ../../../../externals/TinyXPath/android
	if [ ! -d $(OUT_DIR) ]; then mkdir -p $(OUT_DIR); fi;
	cp -Rv ../../../../externals/TinyXPath/android/libs/armeabi/*.so $(OUT_DIR)
	cp -Rv ../../../../util/cpp/android/libs/armeabi/*.so $(OUT_DIR)
	cp -Rv ../../lcppdc/android/libs/armeabi/*.so $(OUT_DIR)
	cp -Rv ../../milstd2525/android/libs/armeabi/*.so $(OUT_DIR)
	cp -Rv ../../c45/android/libs/armeabi/*.so $(OUT_DIR)

build: prebuild
	ndk-build

clean:
	rm -rf libs obj

cleanall: clean
	make -C ../../../../externals/TinyXPath/android clean
	make -C ../../../../util/cpp/android cleanall
	make -C ../../lcppdc/android cleanall
	make -C ../../milstd2525/android cleanall
	make -C ../../c45/android cleanall
