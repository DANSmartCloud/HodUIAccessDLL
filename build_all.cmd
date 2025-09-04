:: Developed By DirRain
@echo off
setlocal enableextensions

echo [Build] Locating MSBuild...
set "MSBUILD="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
  for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -find MSBuild\**\Bin\MSBuild.exe`) do (
    set "MSBUILD=%%i"
    goto :msbuild_found
  )
)

:msbuild_fallback
if not defined MSBUILD (
  if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=C:\Program Files\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
)
if not defined MSBUILD (
  if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
)

:msbuild_found
if not defined MSBUILD (
  echo [Error] MSBuild not found. Please install Visual Studio Build Tools 2022.
  exit /b 1
)
echo [Build] MSBuild: "%MSBUILD%"

echo [Env] Trying to initialize VS developer environment (x86)...
set "VSDEVCMD="
if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" set "VSDEVCMD=C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
if not defined VSDEVCMD if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" set "VSDEVCMD=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
if defined VSDEVCMD (
  call "%VSDEVCMD%" -arch=x86
) else (
  echo [Warn] VsDevCmd.bat not found. Proceeding without it...
)

echo [Build] Building UIAccessDLL (Release^|Win32)...
"%MSBUILD%" .\UIAccessDLL\UIAccessDLL.vcxproj /t:Build /p:Configuration=Release /p:Platform=Win32 || goto :fail

echo [Build] Building TestDLL (Release^|Win32)...
"%MSBUILD%" .\UIAccessDLL\TestDLL.vcxproj /t:Build /p:Configuration=Release /p:Platform=Win32 || goto :fail

echo [Build] Building C++ sample (Release^|Win32)...
"%MSBUILD%" .\examples\CppSample\UIAccessCppSample.vcxproj /t:Build /p:Configuration=Release /p:Platform=Win32 || goto :fail

echo [Post] Copy UIAccessDLL.dll to sample output...
copy /Y ".\UIAccessDLL\Release\UIAccessDLL.dll" ".\examples\CppSample\Release\" >nul

echo [Build] Building uiaccess app (Release^|Win32)...
"%MSBUILD%" .\uiaccess\uiaccess.vcxproj /t:Build /p:Configuration=Release /p:Platform=Win32 || goto :fail

echo [OK] All projects built successfully.
exit /b 0

:fail
echo [FAIL] Build failed with errorlevel %ERRORLEVEL%.
exit /b %ERRORLEVEL%