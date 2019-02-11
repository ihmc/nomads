@ECHO OFF

SET PROTO_VER=3.6.1
SET BIN=..\..\..\..\externals-win\protobuf\%PROTO_VER%\bin\protoc.exe
SET SRC_DIR=..\..\..\..\..\measure
SET OUT_DIR=..
SET TMP_DIR=temp

if not exist %TMP_DIR% mkdir %TMP_DIR%
for %%f in (%SRC_DIR%\*.proto) do (
	echo "Compiling %%f"
	%BIN% --cpp_out=%TMP_DIR% -I%SRC_DIR% %%f
)

for %%f in (%TMP_DIR%/*) do (
	if not exist %OUT_DIR%/%%f (
		echo "Copying %TMP_DIR%\%%f into %OUT_DIR%\%%f"
		xcopy "%TMP_DIR%\%%f" "%OUT_DIR%"
	) else (
		fc "%TMP_DIR%\%%f" "%OUT_DIR%\%%f" > nul
		if errorlevel 1 (
			echo "Copying %TMP_DIR%\%%f into %OUT_DIR%\%%f"
			xcopy "%TMP_DIR%\%%f" "%OUT_DIR%\" /Y
		) else (
			echo "%OUT_DIR%\%%f already up-to-date; nothing to do."
		)
	)
)

echo "Cleaning temp folder..."
@RD /S /Q %TMP_DIR%