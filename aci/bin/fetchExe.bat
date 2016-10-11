rm acik.exe
rm acinative.dll
rm avmnative.dll
pushd ..\cpp\kernel\win32\Debug\
pwd
cp acik-xlayer.exe ..\..\..\..\bin\
cp acinative.dll ..\..\..\..\bin\
cp avmnative.dll ..\..\..\..\bin\
popd
mv acik-xlayer.exe acik.exe
