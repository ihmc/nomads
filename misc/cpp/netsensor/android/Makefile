ARM=armeabi
X86=x86

OUT_DIR=obj/local

all: build

prebuild:
	make -C ../../../../util/cpp/android
	if [ ! -d $(OUT_DIR)/$(ARM) ]; then mkdir -p $(OUT_DIR)/$(ARM); fi;
	cp -Rv ../../../../util/cpp/android/libs/$(ARM)/*.so $(OUT_DIR)/$(ARM)

build: prebuild
	ndk-build

clean:
	rm -rf libs obj

cleanall: clean
	make -C ../../../../../util/cpp/android cleanall
