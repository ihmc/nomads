@echo off

SET PROTO_VER=3.1.0
SET BIN=..\..\..\..\externals-win\protobuf\protobuf-%PROTO_VER%\bin\protoc.exe
REM SET BIN=build\protobuf-%PROTO_VER%\bin\protoc.exe
SET SRC_DIR=measure\
SET OUT_DIR=..\

for %%f in (%SRC_DIR%*) do (
	echo "Compiling %%f"
	%BIN% --cpp_out=%OUT_DIR% -I%SRC_DIR% %%f
)