all: cnats

cnats:
	if [ ! -d build ]; then mkdir build && cd build; else cd build; fi &&\
	cmake .. -DNATS_BUILD_WITH_TLS=OFF && \
	make install
	cp install/lib/libnats_static.a linux/

clean:
	rm -rf build
