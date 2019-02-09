@echo off

set PROTO_BIN="..\..\..\..\externals-win\protobuf\protobuf-3.1.0\bin"
set IMPORTS="..\..\..\..\externals-win\protobuf\protobuf-3.1.0\include"
set SRC_DIR="..\..\..\..\..\measure"
set DST_DIR="..\comm"

%PROTO_BIN%\protoc.exe --proto_path=%IMPORTS% -I=%SRC_DIR% --cpp_out=%DST_DIR% %SRC_DIR%\subject.proto
%PROTO_BIN%\protoc.exe --proto_path=%IMPORTS% -I=%SRC_DIR% --cpp_out=%DST_DIR% %SRC_DIR%\measure.proto
%PROTO_BIN%\protoc.exe --proto_path=%IMPORTS% -I=%SRC_DIR% --cpp_out=%DST_DIR% %SRC_DIR%\subjects\disservice\disservice.proto
