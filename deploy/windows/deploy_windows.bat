@echo off
setlocal EnableDelayedExpansion

:: Set project paths (relative to this script location)
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%..\..\"
set "QT_PATH=C:\Qt\6.9.1\msvc2022_64"
set "BUILD_DIR=%PROJECT_ROOT%build"
set "RELEASE_DIR=%BUILD_DIR%\bin\Release"
set "DEPLOY_DIR=%PROJECT_ROOT%deploy\package\windows"

echo ================================================
echo         CDC Control System Deployment Tool
echo ================================================

:: Check Qt path exists
if not exist "%QT_PATH%" (
    echo Error: Qt path not found at %QT_PATH%
    echo Please modify QT_PATH variable in this script
    pause
    exit /b 1
)

:: Check if build completed
if not exist "%RELEASE_DIR%\CDC_Control_System.exe" (
    echo Error: Executable not found
    echo Please build first: cmake --build . --config Release
    echo Expected location: %RELEASE_DIR%\CDC_Control_System.exe
    pause
    exit /b 1
)

:: Create deployment directory
echo Creating deployment directory...
if exist "%DEPLOY_DIR%" rmdir /s /q "%DEPLOY_DIR%"
mkdir "%DEPLOY_DIR%" 2>nul

:: Copy main executable
echo Copying main executable...
copy "%RELEASE_DIR%\CDC_Control_System.exe" "%DEPLOY_DIR%\"
if %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to copy executable
    pause
    exit /b 1
)

:: Copy configuration files
echo Copying configuration files...
copy "%PROJECT_ROOT%deploy\package\qt.conf" "%DEPLOY_DIR%\"

:: Copy icon if exists
if exist "%SCRIPT_DIR%app.ico" (
    copy "%SCRIPT_DIR%app.ico" "%DEPLOY_DIR%\"
)

:: Copy resources if they exist
if exist "%PROJECT_ROOT%resources" (
    echo Copying resources...
    xcopy "%PROJECT_ROOT%resources" "%DEPLOY_DIR%\resources\" /E /I /Q >nul 2>&1
)

:: Change to deployment directory for windeployqt
cd /d "%DEPLOY_DIR%"

:: Run windeployqt with appropriate options
echo Running Qt deployment tool...
echo Command: "%QT_PATH%\bin\windeployqt.exe" --release --no-translations --no-system-d3d-compiler --no-opengl-sw CDC_Control_System.exe

"%QT_PATH%\bin\windeployqt.exe" ^
    --release ^
    --no-translations ^
    --no-system-d3d-compiler ^
    --no-opengl-sw ^
    --no-quick-import ^
    --no-qmltooling ^
    --serialport ^
    CDC_Control_System.exe

if %ERRORLEVEL% NEQ 0 (
    echo Error: windeployqt execution failed
    pause
    exit /b 1
)

:: Create launcher script
echo Creating launcher script...
echo @echo off > "%DEPLOY_DIR%\run_CDC_Control_System.bat"
echo cd /d "%%~dp0" >> "%DEPLOY_DIR%\run_CDC_Control_System.bat"
echo echo Starting CDC Control System... >> "%DEPLOY_DIR%\run_CDC_Control_System.bat"
echo CDC_Control_System.exe >> "%DEPLOY_DIR%\run_CDC_Control_System.bat"
echo if %%ERRORLEVEL%% NEQ 0 ( >> "%DEPLOY_DIR%\run_CDC_Control_System.bat"
echo     echo Application exited with error code: %%ERRORLEVEL%% >> "%DEPLOY_DIR%\run_CDC_Control_System.bat"
echo     pause >> "%DEPLOY_DIR%\run_CDC_Control_System.bat"
echo ^) >> "%DEPLOY_DIR%\run_CDC_Control_System.bat"

:: Create README for deployment package
echo Creating deployment README...
echo CDC Control System - Deployment Package > "%DEPLOY_DIR%\README.txt"
echo ======================================== >> "%DEPLOY_DIR%\README.txt"
echo. >> "%DEPLOY_DIR%\README.txt"
echo This package contains the CDC Control System application >> "%DEPLOY_DIR%\README.txt"
echo and all required Qt dependencies. >> "%DEPLOY_DIR%\README.txt"
echo. >> "%DEPLOY_DIR%\README.txt"
echo To run the application: >> "%DEPLOY_DIR%\README.txt"
echo 1. Double-click run_CDC_Control_System.bat >> "%DEPLOY_DIR%\README.txt"
echo 2. Or directly run CDC_Control_System.exe >> "%DEPLOY_DIR%\README.txt"
echo. >> "%DEPLOY_DIR%\README.txt"
echo System Requirements: >> "%DEPLOY_DIR%\README.txt"
echo - Windows 10/11 >> "%DEPLOY_DIR%\README.txt"
echo - Visual C++ Redistributable 2022 (x64) >> "%DEPLOY_DIR%\README.txt"
echo. >> "%DEPLOY_DIR%\README.txt"
echo Developer: RuiYing, TUM LSE Laboratory >> "%DEPLOY_DIR%\README.txt"
echo Build Date: %DATE% %TIME% >> "%DEPLOY_DIR%\README.txt"

:: Calculate package size
for /f "tokens=3" %%i in ('dir "%DEPLOY_DIR%" /-c ^| findstr /C:" bytes"') do set PACKAGE_SIZE=%%i

:: Display deployment information
echo.
echo ================================================
echo             Deployment Complete!
echo ================================================
echo Deployment directory: %DEPLOY_DIR%
echo Package size: %PACKAGE_SIZE% bytes
echo.
echo Package contents:
dir "%DEPLOY_DIR%" /B

echo.
echo You can now:
echo 1. Run run_CDC_Control_System.bat to test the application
echo 2. Copy the entire 'windows' folder to other computers
echo 3. Create installer or ZIP package for distribution
echo 4. Upload to shared drive or distribution platform
echo.

:: Test deployment option
choice /C YN /M "Would you like to test run the deployed application"
if %ERRORLEVEL%==1 (
    echo Testing deployed application...
    start "" "%DEPLOY_DIR%\CDC_Control_System.exe"
    timeout /t 3 >nul
    echo If the application started successfully, deployment is complete!
)

echo.
echo Deployment script finished successfully!
pause