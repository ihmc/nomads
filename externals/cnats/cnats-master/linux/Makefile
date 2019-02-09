BUILD_FOLDER = ../build
INSTALL_FOLDER = ../install
MACHINE = $(shell uname -m)
ifeq ($(MACHINE), x86_64)
ARCH = 64
else
ARCH = 32
endif
# Possible build types: Release, Debug, RelWithDebInfo, MinRelSize
BUILD_TYPE = Release


all: libnats

libnats:
	mkdir -p $(BUILD_FOLDER)
	cd $(BUILD_FOLDER) && cmake .. -DNATS_BUILD_ARCH=$(ARCH) \
	-DNATS_BUILD_TYPE=$(BUILD_TYPE) -DNATS_INSTALL_PREFIX=$(INSTALL_FOLDER) && \
	make && make install

clean:
	rm -rf $(BUILD_FOLDER)
	rm -rf $(INSTALL_FOLDER)

