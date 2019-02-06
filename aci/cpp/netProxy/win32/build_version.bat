@echo OFF
set ver_file=../version.cpp
echo #include "version.h"> %ver_file%
echo(>> %ver_file%

echo const char * _BUILD_DATE = __DATE__;>> %ver_file%
echo const char * _BUILD_TIME = __TIME__;>> %ver_file%

FOR /F "tokens=* USEBACKQ" %%F IN (`git rev-parse --abbrev-ref HEAD`) DO (SET branch=%%F)
echo const char * _GIT_BRANCH = "%branch%";>> %ver_file%

FOR /F "tokens=* USEBACKQ" %%F IN (`git rev-parse --verify HEAD`) DO (SET commit=%%F)
echo const char * _GIT_SHA = "%commit%";>> %ver_file%

FOR /F "tokens=* USEBACKQ" %%F IN (`git --no-pager describe --tags --always`) DO (SET git_tag=%%F)
echo const char * _GIT_TAG = "%git_tag%";>> %ver_file%

SETLOCAL ENABLEDELAYEDEXPANSION
SET i=1
FOR /F "tokens=* USEBACKQ" %%F IN (`git --no-pager show --date^=iso --format^="%%ad" --name-only`) DO (
  SET git_date!i!=%%F
  SET /a i=!i!+1
)
echo const char * _GIT_DATE = "%git_date1%";>> %ver_file%
ENDLOCAL