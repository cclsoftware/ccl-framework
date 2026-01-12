@echo off
setlocal enabledelayedexpansion

rem --- This script creates an MSIX Bundle from already compiled binaries ---
rem ---
rem --- Usage:   build_msixbundle.bat [/store] [buildpath] [buildfile] [packagingfile] [msixbundle]
rem --- Example: build_msixbundle.bat ..\..\..\..\..\build\cmake\win\x64\Release ..\..\..\..\..\buildnumber.h PackagingLayout.xml MyApp.msixbundle

if "%~4" == "" (
  echo Usage:   build_msixbundle.bat [/store] [buildpath] [buildfile] [packagingfile] [msixbundle]
  echo Example: build_msixbundle.bat ..\..\..\..\..\build\cmake\win\x64\Release ..\..\..\..\..\buildnumber.h PackagingLayout.xml MyApp.msixbundle
  goto exit
)

set scriptpath=%~dp0

set basepath=%~dp0\..\..\..
set toolpath=%basepath%\tools\bin\win

set msixtool="%toolpath%\msixtools\makeappx.exe"
set pritool="%toolpath%\msixtools\makepri.exe"
set sevenzip=%toolpath%\7za.exe

if '%~1' == '/store' (
	set storearg=%~1
	shift
)

set architectures=x64

if '%~1' == '/arch' (
	set architectures=%~2
	shift
	shift
)

set builddir=%1

call %scriptpath%\generate_manifest.bat %storearg% /arch %architectures% %2
call %scriptpath%\generate_layout.bat /arch %architectures% %1

set script=         $version=(Select-String -path AppxManifest.xml -Pattern 'Identity .* Version=\"([0-9\.]+)\"').matches.groups[1].value
set script=%script%;if (Test-Path -Path priconfig.xml -PathType Leaf) { %pritool% new /cf priconfig.xml /pr . /mn AppxManifest.xml /o /of resources.pri }
set script=%script%;%msixtool% build /f "%~3" /op build /bv $version /pv $version /o

powershell -Command "%script%"

set output=%4
if "!output:msixupload=!" == "!output!" (
  copy /b /y build\*.msixbundle %4 > nul
) else (
  cd build
  for /f "tokens=*" %%d in ('dir /b *.msixbundle') do (set appxsym=%%d)
  set appxsym=!appxsym:msixbundle=appxsym!
  cd ..

  :architectures
  if "!architectures!" == "" ( goto compress )
  for /f "tokens=1,* delims=+" %%a in ("!architectures!") do (
    set architecture=%%a
    set dir=%builddir:{CPUTYPE}=!architecture!%

    call collectsymbols.bat /arch !architecture! !dir!

    set architectures=%%b
  )
  goto architectures

  :compress
  echo Compressing debug symbols...
  del build\!appxsym! 2> nul
  %SEVENZIP% -tzip a build\!appxsym! .\build\symbols\* > nul

  echo Creating upload package...
  del !output! 2> nul
  %sevenzip% -tzip a !output! .\build\*.msixbundle .\build\*.appxsym > nul
)

rem --- Clean up ---
:cleanup
echo Cleaning up...
rmdir /s /q build

:exit

endlocal
exit /b
