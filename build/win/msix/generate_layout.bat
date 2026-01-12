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

set script=         $builddir='%~1'

set script=%script%;$manifest=(gc PackagingLayout.xml.template)
set script=%script%;$manifest=$manifest.replace('{BUILDDIR}', $builddir)

:architectures
if "!architectures!" == "" ( goto continue )
for /f "tokens=1,* delims=+" %%a in ("!architectures!") do (
    set architecture=%%a
	set cputype=!architecture:arm64ec=arm64!

    set template=PackagingLayout-generic.xml.template
    if exist "PackagingLayout-!architecture!.xml.template" (
        set template=PackagingLayout-!architecture!.xml.template
    )

    if exist "!template!" (
        set script=!script!;$builddir='%~1'
        set script=!script!;$builddir="$builddir.replace('{CPUTYPE}', '!architecture!')"

        set script=!script!;$manifest_!architecture!="(gc -raw !template!)"
        set script=!script!;$manifest_!architecture!="$manifest_!architecture!.replace('{BUILDDIR}', $builddir).replace('{ARCHITECTURE}', '!architecture!').replace('{CPUTYPE}', '!cputype!')"

        set script=!script!;$manifest="$manifest.replace('{ARCHITECTURE_PACKAGES}', $manifest_!architecture! + \"`r`n{ARCHITECTURE_PACKAGES}\")"
    )

    set architectures=%%b
)
goto architectures

:continue
set script=%script%;$manifest=$manifest.replace('{ARCHITECTURE_PACKAGES}', '')

set script=%script%;Out-File -inputobject $manifest -encoding utf8 PackagingLayout.xml

powershell -Command "%script%"
