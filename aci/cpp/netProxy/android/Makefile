OUT_DIR=obj/local/armeabi

all: build

prebuild:
	make -C ../../../../externals/msgpack/android
	make -C ../../../../mockets/cpp/jni/android
	if [ ! -d $(OUT_DIR) ]; then mkdir -p $(OUT_DIR); fi;
	cp -Rv ../../../../externals/msgpack/android/libs/armeabi/*.so $(OUT_DIR)
	cp -Rv ../../../../mockets/cpp/jni/android/libs/armeabi/*.so $(OUT_DIR)
	
build: prebuild
	ndk-build

clean:
	rm -rf libs obj

cleanall: clean
	make -C ../../../../externals/msgpack/android clean
	make -C ../../../../util/cpp/android clean
	make -C ../../../../mockets/cpp/android clean	
