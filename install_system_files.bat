@echo off

echo Copying %1\libpvrclient64.dll...
copy %1\libpvrclient64.dll %SystemRoot%\system32\
if errorlevel 1 goto :error

echo Copying %1\jsoncpp.dll...
copy %1\jsoncpp.dll %SystemRoot%\system32\
if errorlevel 1 goto :error

echo Copying %1\libzmq-mt-*4_3_3.dll...
copy %1\libzmq-mt-*4_3_3.dll %SystemRoot%\system32\
if errorlevel 1 goto :error

echo Copying %1\varjolib.dll...
copy %1\varjolib.dll %SystemRoot%\system32\
if errorlevel 1 goto :error

echo "All done!"
exit /b 0

:error
echo "Installation failed :-("
pause
exit /b 1
