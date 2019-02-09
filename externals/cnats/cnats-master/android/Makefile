OUT_DIR=obj/local/armeabi

all: build

prebuild:
	if [ ! -d $(OUT_DIR) ]; then mkdir -p $(OUT_DIR); fi;


build: prebuild
	ndk-build

clean:
	rm -rf libs obj

cleanall: clean
