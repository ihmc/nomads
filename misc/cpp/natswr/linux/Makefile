include Makefile.inc

build_dbg=cmake-build-debug

all: natswr 

libutil.a:
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libutil.a

libsecurity.a:
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libsecurity.a

natswr:
	mkdir -p ${build_dbg}
	-(cd ${build_dbg}; cmake ../.. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug && cmake --build .)
	mv ${build_dbg}/libnatswr.a .

natswr_pub: natswr
	mv ${build_dbg}/natswr_pub .

natswr_sub: natswr
	mv ${build_dbg}/natswr_sub .

clean:
	rm -rf ${build_dbg} libnatswr.a natswr_pub natswr_sub
