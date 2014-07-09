echo ********************************
echo ** Clean up previous runs
echo ********************************

rmdir /q /s %BUILD_HOME%
rmdir /q /s %DIST_HOME%

echo ********************************
echo ** Copy source to build dir
echo ********************************

mkdir %BUILD_HOME%
xcopy /E /Q %SOURCE_HOME% %BUILD_HOME%\

