@echo off
setlocal enabledelayedexpansion

rem --- This script combines multiple MSIX Bundles into a single bundle ---
rem ---
rem --- Usage:   combine_msixbundle.bat [input files] [msixbundle]
rem --- Example: combine_msixbundle.bat *.msixbundle MyApp.msixbundle

if "%~2" == "" (
  echo Usage:   combine_msixbundle.bat [input files] [msixbundle]
  echo Example: combine_msixbundle.bat *.msixbundle MyApp.msixbundle
  goto exit
)

set scriptpath=%~dp0

set basepath=%~dp0\..\..\..
set toolpath=%basepath%\tools\bin\win

set msixtool="%toolpath%\msixtools\makeappx.exe"
set sevenzip=%toolpath%\7za.exe

set output=%2

rem --- Extract MSIX packages ---
echo Extracting files...

for %%f in (%1) do (
  set file=%%f
  set file=!file:"=!

  echo   !file!...
  if "!file:msixupload=!" == "!file!" (
    %sevenzip% e -aoa -obuild "!file!" *.msix > nul
    %sevenzip% e -aoa -omanifest "!file!" AppxMetadata\AppxBundleManifest.xml > nul
  ) else (
    %sevenzip% e -aoa -obuild "!file!" *.msixbundle > nul
    %sevenzip% e -aoa -obuild build\*.msixbundle *.msix > nul
    %sevenzip% e -aoa -omanifest build\*.msixbundle AppxMetadata\AppxBundleManifest.xml > nul

    %sevenzip% e -aoa -oappxsym "!file!" *.appxsym > nul
    %sevenzip% x -aoa -oappxsym appxsym\*.appxsym > nul

    del build\*.msixbundle
    del appxsym\*.appxsym
  )
)

rem --- Create bundle ---
echo Creating bundle...

set script=         $version=(Select-String -path manifest\AppxBundleManifest.xml -Pattern 'Identity .* Version=\"([0-9\.]+)\"').matches.groups[1].value
set script=%script%;%msixtool% bundle /d build /p '!output:msixupload=msixbundle!' /bv $version /o

powershell -Command "%script%"

if "!output:msixupload=!" neq "!output!" (
  echo Compressing debug symbols...
  del build\bundle.appxsym 2> nul
  %sevenzip% -tzip a !output:msixupload=appxsym! .\appxsym\* > nul

  echo Creating upload package...
  del !output! 2> nul
  %sevenzip% -tzip a !output! !output:msixupload=msixbundle! !output:msixupload=appxsym! > nul

  del !output:msixupload=msixbundle!
  del !output:msixupload=appxsym!
)

rem --- Clean up ---
:cleanup
echo Cleaning up...
rmdir /s /q build
rmdir /s /q manifest
rmdir /s /q appxsym 2> nul

:exit

endlocal
exit /b
