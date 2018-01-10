ARM=armeabi
ARM_V7=armeabi-v7a
X86=x86

OUT_DIR=obj/local

all: build

prebuild:
	if [ ! -d $(OUT_DIR)/$(ARM) ]; then mkdir -p $(OUT_DIR)/$(ARM); fi;
#	if [ ! -d $(OUT_DIR)/$(ARM_V7) ]; then mkdir -p $(OUT_DIR)/$(ARM_V7); fi;
#	if [ ! -d $(OUT_DIR)/$(X86) ]; then mkdir -p $(OUT_DIR)/$(X86); fi;
#	cp -Rv ../../../android/externals/prebuilt/openssl/*.so $(OUT_DIR)/$(ARM)
	cp -Rv ../../../android/externals/openssl/openssl-1.0.2h/*.so $(OUT_DIR)/$(ARM)

build: prebuild
	ndk-build

clean:
	rm -rf libs obj

cleanall: clean
	rm -rf ../../../android/externals/openssl/libs
	rm -rf ../../../android/externals/openssl/obj
