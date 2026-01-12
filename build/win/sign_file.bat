@echo off

setlocal enabledelayedexpansion

rem ===============================================================================================================================================
rem Configuration
rem ===============================================================================================================================================

set signtool="%~dp0\..\..\tools\bin\win\signtools\signtool.exe"

set timestampserver=http://timestamp.digicert.com
set digest=sha256

set retries=6
set delay=5

set useazure=false

set usehsm=false

:args
if '%1' == '/force' (
	set force=true
	shift
) else if '%1' == '/cert' (
	set cert=%2
	shift
	shift
) else if '%1' == '/client' (
	set client=%2
	set useazure=true
	shift
	shift
) else if '%1' == '/tenant' (
	set tenant=%2
	set useazure=true
	shift
	shift
) else if '%1' == '/keyvault' (
	set keyvault=%2
	set useazure=true
	shift
	shift
) else if '%1' == '/secret' (
	set secret=%2
	set useazure=true
	shift
	shift
) else if '%1' == '/timestampserver' (
	set timestampserver=%2
	shift
	shift
) else if '%1' == '/thumbprint' (
	set	thumbprint=%2
	set	usethumbprint=true
	shift
	shift
) else if '%1' == '/signtool' (
	set signtool=%2
	shift
	shift
) else (
	goto files
)

goto args

:files

if '%useazure%' == 'false' (
	echo *** ATTENTION: %cert%%thumbprint% certificate must be installed on this machine! ***
	echo *** Alternatively, for EV Code Signing you need the SafeNet client installed and the USB Token connected to the local machine! ***
	echo.
)

if '%force%' == 'true' (
	set files=%1
) else (
	set skipped=0

	for %%f in (%1) do (
		set skip=false
		set file=%%f
		set file=!file:"=!

		if exist "!file!.signed" if exist "!file!.signed.cert" (
			for %%i in ("!file!") do set filesize=%%~zi
			set /p signsize=< "!file!.signed"
			set /p signcert=< "!file!.signed.cert"

			if !filesize! == !signsize! if !cert!!thumbprint! == !signcert! (
				for %%i in ("!file!") do set filedate=%%~ti
				for %%i in ("!file!.signed") do set signdate=%%~ti
				for /f "delims=" %%i in ('dir /b /o:d "!file!" "!file!.signed"') do set newest=%%i

				if "!filedate!" == "!signdate!" (
					set skip=true
				) else (
					if "!newest:~-7!" == ".signed" set skip=true
				)
			)
		)

		if "!skip!" == "true" (
			echo *** Target file "!file!" is already signed, skipping. ***
			set /a skipped+=1
		) else (
			set files=!files! "!file!"
		)
	)

	if !skipped! neq 0 echo.
	if "!files!" == "" exit /b 0
)

:try
	if '%useazure%' == 'true' (
		call AzureSignTool sign -v -kvu %keyvault% -kvi %client% -kvt %tenant% -kvs %secret% -kvc %cert% -tr %timestampserver% -mdop 1 !files!
	) else if '%usethumbprint%' == 'true' (
		call %signtool% sign /v /sha1 %thumbprint% /fd %digest% /tr %timestampserver% /td %digest% /sm !files!
	) else (
		call %signtool% sign /v /n %cert% /fd %digest% /tr %timestampserver% /td %digest% /a !files!
	)

	if not errorlevel 1 (
		for %%x in (!files!) do echo %%~zx> "%%x.signed"
		for %%x in (!files!) do echo %cert%%thumbprint%> "%%x.signed.cert"
		goto success
	)

	if %retries%==0 goto fail
	set /a retries-=1

	rem Use ping with a TEST-NET IP to delay execution as the timeout command will not work in non-interactive scripts.
	echo.
	echo *** Code signing failed. Waiting %delay% seconds before retry. ***
	ping 192.0.2.0 -n 1 -w %delay%000 > NUL
	set /a delay*=2
	echo.
	goto try

:success
	exit /b 0

:fail
	exit /b 1
