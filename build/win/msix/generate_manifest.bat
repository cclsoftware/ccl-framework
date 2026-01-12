@echo off
setlocal enabledelayedexpansion

set channel=direct

if '%~1' == '/store' (
	set channel=store
	shift
)

set architectures=x64

if '%~1' == '/arch' (
	set architectures=%~2
	shift
	shift
)

set cputypes=!architectures:arm64ec=arm64!

if "!cputypes:+=!" neq "!cputypes!" (
    set cputypes=x86a64
)
	
set script=         $major="(Select-String -path ../../../source/appversion.h -Pattern '#define VER_MAJOR\s+([0-9]+)').matches.groups[1].value"
set script=%script%;$minor="(Select-String -path ../../../source/appversion.h -Pattern '#define VER_MINOR\s+([0-9]+)').matches.groups[1].value"
set script=%script%;$offset="(Select-String -path ../../../source/appversion.h -Pattern '#define MSSTORE_BUILD_OFFSET\s+([0-9]+)').matches.groups[1].value"
set script=%script%;$build="(Select-String -path '%~1' -Pattern '#define BUILD_REVISION_NUMBER\s+([0-9]+)').matches.groups[1].value"
set script=%script%;$version="$major+'.'+$minor+'.'+([int]$build-[int]$offset)+'.0'"

set script=%script%;$identity="(Select-String -path publisher.properties -Pattern '%channel%.identity=(.*)').matches.groups[1].value"
set script=%script%;$publisher="(Select-String -path publisher.properties -Pattern '%channel%.publisher=(.*)').matches.groups[1].value"

:architectures
if "!architectures!" == "" ( goto continue )
for /f "tokens=1,* delims=+" %%a in ("!architectures!") do (
    set architecture=%%a
	set cputype=!architecture:arm64ec=arm64!

    set template=AppxManifest-!architecture!.xml.template
    if exist "!template!" (
        set script=!script!;$manifest="(gc !template!)"
        set script=!script!;$manifest="$manifest.replace('{IDENTITY}', $identity).replace('{PUBLISHER}', $publisher).replace('{VERSION}', $version).replace('{CPUTYPE}', '!cputype!')"

        set script=!script!;Out-File -inputobject $manifest -encoding utf8 AppxManifest-!architecture!.xml
    )

    set architectures=%%b
)
goto architectures

:continue
if exist "AppxManifest.xml.template" (
    set script=!script!;$manifest="(gc AppxManifest.xml.template)"
    set script=!script!;$manifest="$manifest.replace('{IDENTITY}', $identity).replace('{PUBLISHER}', $publisher).replace('{VERSION}', $version).replace('{CPUTYPE}', '!cputypes!')"

    set script=!script!;Out-File -inputobject $manifest -encoding utf8 AppxManifest.xml
)

powershell -Command "%script%"
