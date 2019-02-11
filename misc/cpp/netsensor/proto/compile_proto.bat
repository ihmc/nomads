@ECHO OFF

SET PROTO_VER=3.6.1
SET PROTO_BIN=..\..\..\..\externals-win\protobuf\%PROTO_VER%\bin\protoc.exe
SET PROTO_INCLUDE=..\..\..\..\externals-win\protobuf\%PROTO_VER%\include\
SET SRC_DIR=..\proto\
SET OUT_DIR=..\


%PROTO_BIN% --cpp_out=%OUT_DIR% container.proto -I=%PROTO_INCLUDE% -I%SRC_DIR%
%PROTO_BIN% --cpp_out=%OUT_DIR% datatype.proto -I=%PROTO_INCLUDE% -I%SRC_DIR%
%PROTO_BIN% --cpp_out=%OUT_DIR% icmpinfo.proto -I=%PROTO_INCLUDE% -I%SRC_DIR%
%PROTO_BIN% --cpp_out=%OUT_DIR% iw.proto -I=%PROTO_INCLUDE% -I%SRC_DIR%
%PROTO_BIN% --cpp_out=%OUT_DIR% measure.proto -I=%PROTO_INCLUDE% -I%SRC_DIR%
%PROTO_BIN% --cpp_out=%OUT_DIR% netproxyinfo.proto -I=%PROTO_INCLUDE% -I%SRC_DIR%
%PROTO_BIN% --cpp_out=%OUT_DIR% prefix.proto -I=%PROTO_INCLUDE% -I%SRC_DIR%
%PROTO_BIN% --cpp_out=%OUT_DIR% rtt.proto -I=%PROTO_INCLUDE% -I%SRC_DIR%
%PROTO_BIN% --cpp_out=%OUT_DIR% subject.proto -I=%PROTO_INCLUDE% -I%SRC_DIR%
%PROTO_BIN% --cpp_out=%OUT_DIR% topology.proto -I=%PROTO_INCLUDE% -I%SRC_DIR%
%PROTO_BIN% --cpp_out=%OUT_DIR% container.proto -I=%PROTO_INCLUDE% -I%SRC_DIR%
%PROTO_BIN% --cpp_out=%OUT_DIR% traffic.proto -I=%PROTO_INCLUDE% -I%SRC_DIR%
