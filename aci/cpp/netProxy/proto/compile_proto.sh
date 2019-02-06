#!/bin/bash

PROTO_VER=3.1.0
ARCH=`sh ../../../../util/scripts/guessArch.sh`

BIN=../../../../externals/protobuf/protobuf-${PROTO_VER}/${ARCH}/protoc/bin/protoc
SRC_DIR=../../../../measure
OUT_DIR=..
TMP_DIR=temp

# Create temp dir
if [ ! -e "${TMP_DIR}" ]
then
    echo "Creating temporary directory: `pwd`/$TMP_DIR"
    mkdir "$TMP_DIR"
fi

# Compile protobuf files
for f in `ls ${SRC_DIR}/*.proto 2> /dev/null`
do
    echo "Compiling ${f}"
    eval "${BIN} --cpp_out=${TMP_DIR} -I${SRC_DIR} ${f}"
done

# Overwrite old files if they differ
for f in `ls ${TMP_DIR}/* 2> /dev/null`
do
    FILE_NAME=$(basename $f)
    if ! `cmp --silent "${TMP_DIR}/${FILE_NAME}" "${OUT_DIR}/${FILE_NAME}"`
    then
        cp "${TMP_DIR}/${FILE_NAME}" "${OUT_DIR}/${FILE_NAME}"
        echo "Updated ${FILE_NAME}"
    fi
done

#Cleanup
echo "Removing temporary directory: `pwd`/${TMP_DIR}"
rm -rf ${TMP_DIR}
