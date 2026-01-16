@echo off
setlocal enabledelayedexpansion

if exist files.txt del files.txt

echo Collecting source files...

for /r %%f in (*.cpp) do (
    set "p=%%f"
    call :fixpath
)

echo Compiling...

g++ -I"C:/Users/Nico/Documents/macroAdvancer/external" @files.txt -o main.exe -l d3d11 -l dxgi -l dxguid -l d3dcompiler_47 -l dwmapi -lgdi32 -luser32 -lole32 -limm32 -lkernel32

if %ERRORLEVEL% neq 0 (
    echo.
    echo Compilation failed!
) else (
    echo.
    echo Compilation successful!
)

echo Done!
exit /b

:fixpath
set "p=%p:\=/%"
echo %p% >> files.txt
exit /b