@echo off
REM build.bat — Build ze for Windows using gcc (MinGW).
REM Usage:
REM   build.bat           Build the project
REM   build.bat clean     Remove build artifacts
REM   build.bat run       Build and run

setlocal enabledelayedexpansion

set CC=gcc
set CFLAGS=-Wall -Wextra -pedantic -std=c99 -Iinclude
set BUILD_DIR=build
set TARGET=%BUILD_DIR%\ze.exe
set PLATFORM=windows

if "%1"=="clean" goto :clean
if "%1"=="run" goto :run

:build
    REM Collect core sources
    set SRCS=
    for /r src %%f in (*.c) do set SRCS=!SRCS! %%f

    REM Collect platform sources
    for /r platforms\%PLATFORM% %%f in (*.c) do set SRCS=!SRCS! %%f

    if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

    echo Compiling...
    %CC% %CFLAGS% -o %TARGET% !SRCS!
    if errorlevel 1 (
        echo Build failed.
        exit /b 1
    )
    echo Built %TARGET%
    goto :eof

:clean
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    echo Cleaned.
    goto :eof

:run
    call :build
    if errorlevel 1 exit /b 1
    echo.
    %TARGET%
    goto :eof
