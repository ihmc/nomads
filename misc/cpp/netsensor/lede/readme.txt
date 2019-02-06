To cross-compile for LEDE systems, copy the content of the "netsensor" directory
into the directory "package/utils/" in your openwrt home folder, e.g., /home/username/lede-sdk.

Run the command:

	make menuconfig
	
and select the target platform and architecture and then tick the "netsensor"
package and its dependencies (libpcap, libstcpp, libpthread, zlib, libnl).
Save the configuration in a file named ".config".

netsensor depends on protobuf version 3.X that could not be automatically installed or simply installed by running 
    opkg install protobuf
this command, in fact, could install protubuf version 2.X that does not fit for netsensor.
In that case lede support protobuf version 3.X and can be cross compiled for the sdk and installed by copying the .ipk
on the device and running "opkg install" referring to the loaded ipk.

Next, it is necessary to compile the build tools.
From the openwrt home directory run the following commands:

	make tools/install
	make toolchain/install

The next step is compiling the project by running the command

	make package/netsensor/compile -j1 V=s

The options -j1 V=s are useful to print debug lines on the console.

You can find the built package in your LEDE home directory, at the path

	bin/packages/<architecture>/base/netsensor_<version>_<architecture>.ipk



