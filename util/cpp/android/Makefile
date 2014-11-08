OUT_DIR=obj/local/armeabi

all: build

prebuild:
	ndk-build -C ../../../android/externals/openssl
	if [ ! -d $(OUT_DIR) ]; then mkdir -p $(OUT_DIR); fi;
	cp -Rv ../../../android/externals/openssl/libs/armeabi/*.so $(OUT_DIR)

build: prebuild
	ndk-build

clean:
	rm -rf libs obj

cleanall: clean
	rm -rf ../../../android/externals/openssl/libs
	rm -rf ../../../android/externals/openssl/obj
