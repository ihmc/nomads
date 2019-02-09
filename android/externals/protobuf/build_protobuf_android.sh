# Path where you want to put the libs
PREFIX=`pwd`/protobuf/android
rm -rf ${PREFIX}
mkdir ${PREFIX}

export NDK=YOUR_NDK_ROOT
 
# 0 Download and install android ndk
# Download android ndk.
# Extract and cd to the ndk root folder.
# Launch the make tool chain script to create a toolchain:
# Change version number accordingly
# sh ./build/tools/make-standalone-toolchain.sh --arch=arm --platform=android-23 --install-dir=/tmp/my-android-toolchain

# 1. Use the tools from the Standalone Toolchain
export PATH=/tmp/my-android-toolchain/bin:$PATH
export SYSROOT=/tmp/my-android-toolchain/sysroot
export CC="arm-linux-androideabi-gcc --sysroot $SYSROOT"
export CXX="arm-linux-androideabi-g++ --sysroot $SYSROOT"
export CXXSTL=$NDK/sources/cxx-stl/gnu-libstdc++/4.6
 
##########################################
# Fetch Protobuf And put the folder in tmp
##########################################
# cd into the protobuf folder
cd tmp
cd protobuf-3.1.0

mkdir build
 
# 3. Run the configure to target a static library for the ARMv7 ABI
# for using protoc, you need to install protoc to your OS first, or use another protoc by path
./configure --prefix=$(pwd)/build \
--host=arm-linux-androideabi \
--with-sysroot=$SYSROOT \
--disable-shared \
--enable-cross-compile \
--with-protoc=protoc \
CFLAGS="-march=armv7-a" \
CXXFLAGS="-march=armv7-a -I$CXXSTL/include -I$CXXSTL/libs/armeabi-v7a/include"
 
# Note: You may need to install a compatible version of protoc for the tests to work
# you have to install protoc for linux for it to work. 
# 4. Build
make && make install
 
# 5. Inspect the library architecture specific information
arm-linux-androideabi-readelf -A build/lib/libprotobuf-lite.a

cp build/lib/libprotobuf.a $PREFIX/libprotobuf.a
cp build/lib/libprotobuf-lite.a $PREFIX/libprotobuf-lite.a
