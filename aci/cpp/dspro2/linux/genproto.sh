#!/bin/bash

IMPORTS="../../../../externals/protobuf/protobuf-3.1.0/include"
SRC_DIR="../../../../../measure"
DST_DIR="../comm"

protoc --proto_path=$IMPORTS -I=$SRC_DIR --cpp_out=$DST_DIR $SRC_DIR/subject.proto
protoc --proto_path=$IMPORTS -I=$SRC_DIR --cpp_out=$DST_DIR $SRC_DIR/measure.proto
protoc --proto_path=$IMPORTS -I=$SRC_DIR --cpp_out=$DST_DIR $SRC_DIR/subjects/disservice/disservice.proto
