@echo OFF
@setlocal EnableDelayedExpansion

if defined VisualStudioVersion (
  set IDE=1
) else (
  set IDE=0
)

@rem TODO(mtwilliams): Cache environment.

if not defined YETI_COPYRIGHT (call %~dp0\info.bat)
if not defined YETI_VERSION   (call %~dp0\info.bat)
if not defined YETI_REVISION  (call %~dp0\info.bat)

if not defined TOOLCHAIN (
  if defined VisualStudioVersion (
    set TOOLCHAIN=%VisualStudioVersion%
  ) else (
    echo Using latest Visual Studio install... 1>&2
    set TOOLCHAIN=latest
  )
)

call %~dp0\scripts\vc.bat %TOOLCHAIN% windows x86

if not %ERRORLEVEL% EQU 0 (
  echo Could not setup environment for x86!
  exit /B 1
)

pushd %~dp0\..

mkdir _build\obj 2>NUL
mkdir _build\bin 2>NUL
mkdir _build\lib 2>NUL

:build

cl.exe /nologo /c /W4 /arch:IA32 /fp:except /favor:blend /Od /Oi ^
       /Gm- /GR- /EHa- /GS /MDd ^
       /Fo%_build\obj\yeti_debug_windows_32.obj ^
       /Zi /Fd%_build\yeti_debug_windows_32.pdb ^
       /D__YETI_COPYRIGHT__="\"%YETI_COPYRIGHT%\"" ^
       /D__YETI_VERSION__="\"%YETI_VERSION%\"" ^
       /D__YETI_REVISION__="%YETI_REVISION%" ^
       /DYETI_CONFIGURATION=YETI_CONFIGURATION_DEBUG ^
       /DYETI_LINKAGE=YETI_LINKAGE_STATIC ^
       /DLOOM_CONFIGURATION=LOOM_CONFIGURATION_DEBUG ^
       /DLOOM_LINKAGE=LOOM_LINKAGE_STATIC ^
       /DGALA_CONFIGURATION=GALA_CONFIGURATION_DEBUG ^
       /DGALA_LINKAGE=GALA_LINKAGE_STATIC ^
       /I_deps\luajit\include ^
       /I_deps\sqlite3\include ^
       /I_deps\loom\include ^
       /I_deps\gala\include ^
       /Iinclude /Isrc ^
       src\build.cc

if not %ERRORLEVEL% equ 0 (
  popd
  echo Compilation failed.
  exit /B 1
)

lib.exe /nologo /machine:X86 ^
        /out:_build\lib\yeti_debug_windows_32.lib ^
        _build\obj\yeti_debug_windows_32.obj

echo Built `yeti_debug_windows_32.lib`.

popd
