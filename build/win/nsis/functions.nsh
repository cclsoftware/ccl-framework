!include "StrFunc.nsh"
${Using:StrFunc} StrTok
${Using:StrFunc} StrRep

!include "FileFunc.nsh"

;-----------------------------------------------------------------------
; Shared functions for all installer scripts
;-----------------------------------------------------------------------

Function CheckWindowsVersion
	; Check for correct architecture
	!ifdef ARM64
	${IfNot} ${IsNativeARM64}
		MessageBox MB_ICONEXCLAMATION|MB_OK "$(WindowsArm64)"
		Quit
	${EndIf}
	!else
	!ifdef ARM64X
	${IfNot} ${IsNativeARM64}
		MessageBox MB_ICONEXCLAMATION|MB_OK "$(WindowsArm64)"
		Quit
	${EndIf}
	!else
	!ifdef ARM64EC
	${IfNot} ${IsNativeARM64}
		MessageBox MB_ICONEXCLAMATION|MB_OK "$(WindowsArm64)"
		Quit
	${ElseIfNot} ${AtLeastWin11}
	${OrIfNot} ${AtLeastBuild} 22621
		MessageBox MB_ICONEXCLAMATION|MB_OK "$(Windows11OrLater)"
		Quit
	${EndIf}
	!endif
	!endif
	!endif

	; Check for Windows 10/11 22H2 or later
	${IfNot} ${AtLeastWin10}
	${OrIfNot} ${AtLeastBuild} 19045
		MessageBox MB_ICONEXCLAMATION|MB_OK "$(Windows10OrLater)"
		Quit
	${ElseIf} ${AtLeastWin11}
	${AndIfNot} ${AtLeastBuild} 22621
		MessageBox MB_ICONEXCLAMATION|MB_OK "$(Windows10OrLater)"
		Quit
	${EndIf}
FunctionEnd

Function Check64BitVersion
	; Switch to x64 Registry
	!ifdef WIN64
	${IfNot} ${RunningX64}
		MessageBox MB_ICONEXCLAMATION|MB_OK "$(Windows64)"
		Quit
	${EndIf}
	SetRegView 64
	!endif
FunctionEnd

Function TrimWhitespace
	Exch $R1
	Push $R2
Loop:
	StrCpy $R2 "$R1" 1
	StrCmp "$R2" " " TrimLeft
	StrCmp "$R2" "$\r" TrimLeft
	StrCmp "$R2" "$\n" TrimLeft
	StrCmp "$R2" "$\t" TrimLeft
	GoTo Loop2
TrimLeft:
	StrCpy $R1 "$R1" "" 1
	Goto Loop
Loop2:
	StrCpy $R2 "$R1" 1 -1
	StrCmp "$R2" " " TrimRight
	StrCmp "$R2" "$\r" TrimRight
	StrCmp "$R2" "$\n" TrimRight
	StrCmp "$R2" "$\t" TrimRight
	GoTo Done
TrimRight:
	StrCpy $R1 "$R1" -1
	Goto Loop2
Done:
	Pop $R2
	Exch $R1
FunctionEnd

!define TrimWhitespace "!insertmacro TrimWhitespace"
!macro TrimWhitespace resultVar string
	Push "${string}"
	Call TrimWhitespace
	Pop "${resultVar}"
!macroend

Function GetFirewallProductName
	nsexec::ExecToStack "wmic /NAMESPACE:\\root\SecurityCenter2 path FirewallProduct get displayName"
	Pop $0
	Pop $1

	${If} $0 == 0
		${StrTok} $1 $1 "$\r$\n" 1 1
		${TrimWhitespace} $0 $1
	${Else}
		StrCpy $0 ""
	${EndIf}
FunctionEnd

Function Check3rdPartyFirewall
	Call GetFirewallProductName
	${If} $0 != ""
		MessageBox MB_ICONEXCLAMATION|MB_OK "$(FirewallWarning)"
	${EndIf}
FunctionEnd

Function UninstallExistingProduct
	Pop $1 ; hint
	Exch $0 ; product

	; Get the UninstallString of the existing installation from the registry. This points to either
	; the wrapper executable (Uninstaller.exe) or the real installer executable (Uninstall.exe). We
	; want to run only the latter.

	ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$0" "UninstallString"
	${StrRep} $0 $0 "Uninstaller.exe" "Uninstall.exe"
	${GetParent} $0 $2

	; At this point $0 is the empty string or the path to the uninstaller executable.
	; Proceed only if the file actually exists.

	IfFileExists $0 0 NotFound
	DetailPrint $1
	SetDetailsPrint none
	ExecWait '"$0" /S _?=$2' $0
	SetDetailsPrint both
	GoTo Exit
NotFound:
	; Product not found, return success
	StrCpy $0 "0"
Exit:
	; Push exit code to stack, restore previous value of $0
	Exch $0
FunctionEnd

!define UninstallExistingProduct "!insertmacro UninstallExistingProduct"
!macro UninstallExistingProduct resultVar product hint
	Push "${product}"
	Push "${hint}"
	Call UninstallExistingProduct
	; Pop exit code from stack, store in resultVar
	Pop "${resultVar}"
!macroend

!macro AddFileAssociation extension key title iconResourceId
	WriteRegStr HKCR ".${extension}" "" "${key}"
	${If} "${title}" != ""
		WriteRegStr HKCR "${key}" "" "${title}"
		WriteRegStr HKCR "${key}\DefaultIcon" "" "$INSTDIR\${EXENAME},-${iconResourceId}"
	${EndIf}
!macroend

!macro AddFileAssociationAndCommand extension key title iconResourceId
	!insertmacro AddFileAssociation "${extension}" "${key}" "${title}" "${iconResourceId}"
	WriteRegStr HKCR "${key}\shell" "" "open"
	WriteRegStr HKCR "${key}\shell\open\command" "" '"$INSTDIR\${EXENAME}" "%1"'
!macroend

!macro AddURIAssociation scheme title iconResourceId
	WriteRegStr HKCR "${scheme}" "" "URL:${title}"
	WriteRegStr HKCR "${scheme}" "URL Protocol" ""
	WriteRegStr HKCR "${scheme}\DefaultIcon" "" "$INSTDIR\${EXENAME},-${iconResourceId}"

	WriteRegStr HKCR "${scheme}\shell" "" ""
	WriteRegStr HKCR "${scheme}\shell\open\command" "" '"$INSTDIR\${EXENAME}" "%1"'
!macroend
