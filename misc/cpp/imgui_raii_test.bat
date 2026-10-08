@echo off
REM dear imgui: build + run the imgui_raii.h / imgui_raii_widgets.h smoke test.
REM Headless (null backend), no GPU needed.
setlocal
set ROOT=%~dp0..\..
set OUT=%TEMP%\imgui_raii_test.exe

where clang++ >nul 2>nul
if %ERRORLEVEL%==0 (set CC=clang++) else (set CC=g++)

echo Building with %CC%...
%CC% -std=c++17 -O1 -g -Wall -Wextra -Wno-unused-function ^
  -I "%ROOT%" ^
  "%ROOT%\imgui.cpp" "%ROOT%\imgui_draw.cpp" "%ROOT%\imgui_tables.cpp" "%ROOT%\imgui_widgets.cpp" ^
  "%ROOT%\backends\imgui_impl_null.cpp" "%~dp0imgui_raii_test.cpp" ^
  -o "%OUT%"
if errorlevel 1 exit /b 1

"%OUT%"
endlocal
