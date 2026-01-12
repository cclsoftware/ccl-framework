;-----------------------------------------------------------------------
; Application Template Installer Script
;-----------------------------------------------------------------------

; Properly display all languages
Unicode true

; DPI-awareness option requires NSIS 3.x!
ManifestDPIAware true

;-----------------------------------------------------------------------
; Build Locations
;-----------------------------------------------------------------------

!ifndef BASEDIR
!define BASEDIR "(NativePathToRoot)"
!endif

!ifndef BUILDDIR
!ifdef X64
!define BUILDDIR "${BASEDIR}\build\win\x64\release"
!else
!define BUILDDIR "${BASEDIR}\build\win\release"
!endif
!endif

!ifndef NSIS_INCLUDES_DIR
!define NSIS_INCLUDES_DIR "${BASEDIR}\framework\build\win\nsis"
!endif

;-----------------------------------------------------------------------
; Includes
;-----------------------------------------------------------------------

!include "MUI2.nsh"
!include "x64.nsh"
!include "WinVer.nsh"

!include "${NSIS_INCLUDES_DIR}\shared.nsh"

;-----------------------------------------------------------------------
; Settings
;-----------------------------------------------------------------------

!define PRODUCT  		"Application Template"
!define EXENAME			"${PRODUCT}.exe"

!searchparse /file ../../../source/appversion.h '#define VER_MAJOR			' VER_MAJOR
!searchparse /file ../../../source/appversion.h '#define VER_MINOR			' VER_MINOR
!searchparse /file ../../../source/appversion.h '#define VER_REVISION		' VER_REVISION
!define PRODUCT_VERSION	"${VER_MAJOR}.${VER_MINOR}.${VER_REVISION}"

Name 			"${PRODUCT}"
OutFile			"${COMPANY} ${PRODUCT} Installer.exe"
InstallDir		"$PROGRAMFILES64\${COMPANY}\${PRODUCT}"
BrandingText	" "

;-----------------------------------------------------------------------
; Version Information
;-----------------------------------------------------------------------

# search buildnumber.h for repository revision
!ifndef BUILD_REVISION
!searchparse /file ${BASEDIR}\buildnumber.h '#define BUILD_REVISION_STRING		"' BUILD_REVISION '"'
!endif

VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey  "CompanyName" "${COMPANY}"
VIAddVersionKey  "FileDescription" "${PRODUCT} Installer"
VIAddVersionKey  "FileVersion" "${PRODUCT_VERSION}.${BUILD_REVISION}"
;VIAddVersionKey  "InternalName" ""
VIAddVersionKey  "LegalCopyright" "${COPYRIGHT}"
VIAddVersionKey  "ProductName" "${PRODUCT}"
VIAddVersionKey  "ProductVersion" "${PRODUCT_VERSION}.${BUILD_REVISION}"

;-----------------------------------------------------------------------
; Definitions
;-----------------------------------------------------------------------

!define MUI_ICON   "${CCL_BASEDIR}\packaging\win\resource\cclapp.ico"
!define MUI_UNICON "${CCL_BASEDIR}\packaging\win\resource\cclapp.ico"

;!define MUI_WELCOMEFINISHPAGE_BITMAP "resource\wizard@2x.bmp"
;!define MUI_UNWELCOMEFINISHPAGE_BITMAP "resource\wizard@2x.bmp"
;!define MUI_ABORTWARNING

;!define MUI_HEADERIMAGE
;!define MUI_HEADERIMAGE_BITMAP "resource\header@2x.bmp"
;!define MUI_HEADERIMAGE_UNBITMAP "resource\header@2x.bmp"

;!define MUI_LANGDLL_REGISTRY_ROOT "HKCU" 
;!define MUI_LANGDLL_REGISTRY_KEY "Software\${COMPANY}\${PRODUCT}" 
;!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

;-----------------------------------------------------------------------
; Pages
;-----------------------------------------------------------------------

!insertmacro MUI_PAGE_WELCOME_CUSTOMIZED
!insertmacro MUI_PAGE_LICENSE_CUSTOMIZED $(licenseFileName)

!insertmacro MUI_PAGE_DIRECTORY_CUSTOMIZED
!insertmacro MUI_PAGE_INSTFILES_CUSTOMIZED
!insertmacro MUI_PAGE_FINISH_CUSTOMIZED

; Uninstaller
!insertmacro MUI_UNPAGE_CONFIRM_CUSTOMIZED
!insertmacro MUI_UNPAGE_INSTFILES_CUSTOMIZED

;-----------------------------------------------------------------------
; Languages
;-----------------------------------------------------------------------

!insertmacro MUI_LANGUAGE "English"
;!insertmacro MUI_LANGUAGE "German"
;!insertmacro MUI_LANGUAGE "Japanese"
;!insertmacro MUI_LANGUAGE "SimpChinese"
;!insertmacro MUI_LANGUAGE "French"
;!insertmacro MUI_LANGUAGE "Spanish"

!define EULADIR "${CCL_IDENTITIES_DIR}\ccl\eula"

LicenseLangString licenseFileName ${LANG_ENGLISH} "${EULADIR}\EULA.txt"
;LicenseLangString licenseFileName ${LANG_GERMAN}  "${EULADIR}\EULA-de.txt"
;LicenseLangString licenseFileName ${LANG_SIMPCHINESE}   "${EULADIR}\EULA-zh.txt"
;LicenseLangString licenseFileName ${LANG_JAPANESE} "${EULADIR}\EULA-ja.txt"
;LicenseLangString licenseFileName ${LANG_FRENCH} "${EULADIR}\EULA-fr.txt"
;LicenseLangString licenseFileName ${LANG_SPANISH} "${EULADIR}\EULA-es.txt"

; Shared Strings and font definitions
!include "${NSIS_INCLUDES_DIR}\localize.nsh"

;-----------------------------------------------------------------------
; Functions
;-----------------------------------------------------------------------

Function .onInit

  ; Check for supported Windows version
  Call CheckWindowsVersion

  ; Check for 64 bit version and switch registry view
  Call Check64BitVersion

  ; Language selection dialog
  ;!insertmacro MUI_LANGDLL_DISPLAY

FunctionEnd

;-----------------------------------------------------------------------
; Installer Sections
;-----------------------------------------------------------------------

Section "-All"

  SetOverwrite ifnewer
  
  ;-----------------------------------------------------------------------
  ; Application Files
  ;-----------------------------------------------------------------------

  SetOutPath "$INSTDIR"
  Delete "$INSTDIR\*.dll" ; remove old EXE and DLL files
  Delete "$INSTDIR\*.exe" 

  File "${BUILDDIR}\${EXENAME}"

  ;SetOutPath "$INSTDIR\help"
  ;File /r "..\..\..\assets\help\*.*"
  
  ;-----------------------------------------------------------------------
  ; Framework Files
  ;-----------------------------------------------------------------------

  SetOutPath "$INSTDIR"

  File "${BUILDDIR}\ccltext.dll"
  File "${BUILDDIR}\cclsystem.dll"
  File "${BUILDDIR}\cclgui.dll"
  ;File "${BUILDDIR}\cclnet.dll"
  ;File "${BUILDDIR}\cclsecurity.dll"
 
  ;-----------------------------------------------------------------------
  ; CRT
  ;-----------------------------------------------------------------------
  
  !include /NONFATAL "${CCL_BASEDIR}\submodules\vcredist\crt.nsh"

  ;-----------------------------------------------------------------------
  ; Language Packs
  ;-----------------------------------------------------------------------

  ;SetOutPath "$INSTDIR\languages"
  ;File /r "${BASEDIR}\translations\projects\xxx\languages\*.langpack"
  
  ;-----------------------------------------------------------------------
  ; PlugIns
  ;-----------------------------------------------------------------------

  Delete "$INSTDIR\Plugins\*.dll" ; cleanup plug-in folder

  SetOutPath "$INSTDIR\Plugins"

  ;File "${BUILDDIR}\plugins\xxx.dll"
   
  ;-----------------------------------------------------------------------
  ; Firewall rules
  ;-----------------------------------------------------------------------
  
  ;liteFirewallW::RemoveRule "$INSTDIR\${EXENAME}" "${PRODUCT}"
  ;liteFirewallW::AddRule "$INSTDIR\${EXENAME}" "${PRODUCT}"
  ;Pop $0

  ;-----------------------------------------------------------------------
  ; Uninstaller
  ;-----------------------------------------------------------------------
 
  SetOutPath "$INSTDIR"
  File "${BUILDDIR}\${UNINSTALLER_EXE}"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  ;-----------------------------------------------------------------------
  ; Shortcuts
  ;-----------------------------------------------------------------------

  CreateShortCut "$SMPROGRAMS\${PRODUCT}.lnk" "$INSTDIR\${EXENAME}"
  CreateShortCut "$DESKTOP\${PRODUCT}.lnk" "$INSTDIR\${EXENAME}"

  ;-----------------------------------------------------------------------
  ; Register Uninstaller
  ;-----------------------------------------------------------------------

  WriteRegStr HKLM "Software\${COMPANY}\${PRODUCT}" "" $INSTDIR
  
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayName" "${PRODUCT}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayVersion" "${PRODUCT_VERSION}.${BUILD_REVISION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "DisplayIcon" "$INSTDIR\${EXENAME},-1"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "Publisher" "${PUBLISHER}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "UninstallString" "$INSTDIR\${UNINSTALLER_EXE}"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "NoModify" "1"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}" "NoRepair" "1"
  
SectionEnd

;-----------------------------------------------------------------------
; Uninstaller
;-----------------------------------------------------------------------

Section "Uninstall"

  ;-----------------------------------------------------------------------
  ; User Settings
  ;-----------------------------------------------------------------------

  RMDir /r "$APPDATA\${COMPANY}\${PRODUCT}"

  ;-----------------------------------------------------------------------
  ; Application Files
  ;-----------------------------------------------------------------------

  Delete "$INSTDIR\*.exe"
  Delete "$INSTDIR\*.dll"

  ;RMDir /r "$INSTDIR\help"
  RMDir /r "$INSTDIR\Plugins"
  ;RMDir /r "$INSTDIR\languages"

  RMDir $INSTDIR ; should be empty now
  
  RMDir "$PROGRAMFILES64\${COMPANY}" ; only if empty!!
  
  DeleteRegKey HKCU "Software\${COMPANY}\${PRODUCT}"
  DeleteRegKey HKLM "Software\${COMPANY}\${PRODUCT}"
  
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}"
  
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run\" "${PRODUCT}"

  SetShellVarContext current

  Delete "$DESKTOP\${PRODUCT}.lnk"
  Delete "$SMPROGRAMS\${PRODUCT}.lnk"

  SetShellVarContext all

  Delete "$DESKTOP\${PRODUCT}.lnk"
  Delete "$SMPROGRAMS\${PRODUCT}.lnk"

  SetShellVarContext current

  ;-----------------------------------------------------------------------
  ; Firewall rules
  ;-----------------------------------------------------------------------

  ;liteFirewallW::RemoveRule "$INSTDIR\${EXENAME}" "${PRODUCT}"
  ;Pop $0

SectionEnd

;-----------------------------------------------------------------------
; Uninstaller Functions
;-----------------------------------------------------------------------

Function un.onInit

  ; switch to x64 Registry
  ${If} ${RunningX64}
	SetRegView 64
  ${EndIf}

  ; Language selection dialog
  !insertmacro MUI_UNGETLANGUAGE
  
FunctionEnd
