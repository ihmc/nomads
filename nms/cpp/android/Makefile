OUT_DIR=obj/local/armeabi

all: build

prebuild:
	make -C ../../../util/cpp/android
#make -C ../../../misc/cpp/nockets/android
	if [ ! -d $(OUT_DIR) ]; then mkdir -p $(OUT_DIR); fi;
	cp -Rv ../../../util/cpp/android/libs/armeabi/*.so $(OUT_DIR)
#cp -Rv ../../../misc/cpp/nockets/android/libs/armeabi/*.so $(OUT_DIR)

build: prebuild
	ndk-build

clean:
	rm -rf libs obj

cleanall: clean
	make -C ../../../util/cpp/android cleanall
#	make -C ../../../misc/cpp/nockets/android cleanall
