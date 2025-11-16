@echo off

echo Copying %1\libpvrclient64.dll...
copy %1\libpvrclient64.dll %SystemRoot%\system32\
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
