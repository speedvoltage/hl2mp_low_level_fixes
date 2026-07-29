@echo off
setlocal
set "ROOT=%~dp0"
if "%MMSOURCE_ROOT%"=="" (
  echo Set MMSOURCE_ROOT to the Metamod:Source 2.0 source tree.
  exit /b 1
)
if "%HL2SDK_ROOT%"=="" (
  echo Set HL2SDK_ROOT to the HL2DM SDK tree or Valve Source SDK src directory.
  exit /b 1
)
cmake -S "%ROOT%" -B "%ROOT%build\windows-x86" -A Win32 -DMMSOURCE_ROOT="%MMSOURCE_ROOT%" -DHL2SDK_ROOT="%HL2SDK_ROOT%"
if errorlevel 1 exit /b %errorlevel%
cmake --build "%ROOT%build\windows-x86" --config Release
if errorlevel 1 exit /b %errorlevel%
cmake --install "%ROOT%build\windows-x86" --config Release --prefix "%ROOT%dist\windows-x86"
