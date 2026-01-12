;-----------------------------------------------------------------------
; NSIS Modern UI Customization
;-----------------------------------------------------------------------

;-----------------------------------------------------------------------
; nsResize plugin path
;-----------------------------------------------------------------------

!addplugindir "${CCL_BASEDIR}\submodules\nsis-plugins\nsResize\Unicode\Plugins"
!addincludedir "${CCL_BASEDIR}\submodules\nsis-plugins\nsResize\Include"

!include /NONFATAL "nsResize.nsh"

;-----------------------------------------------------------------------
; nsResize Welcome page
;-----------------------------------------------------------------------

!macro MUI_PAGE_WELCOME_CUSTOMIZED
	
  !ifdef nsResize_WelcomePage
  !define MUI_PAGE_CUSTOMFUNCTION_SHOW WelcomePage_Show
  !insertmacro MUI_PAGE_WELCOME

  Function WelcomePage_Show
    ${nsResize_WelcomePage} ${MUI_CUSTOMIZE_ADDITIONAL_WIDTH} ${MUI_CUSTOMIZE_ADDITIONAL_HEIGHT}
  FunctionEnd
  !else
  !insertmacro MUI_PAGE_WELCOME  
  !endif

!macroend

;-----------------------------------------------------------------------
; nsResize License page
;-----------------------------------------------------------------------

!macro MUI_PAGE_LICENSE_CUSTOMIZED LicenseFileName

  !ifdef nsResize_LicensePage
  !define MUI_PAGE_CUSTOMFUNCTION_SHOW LicensePage_Show
  !insertmacro MUI_PAGE_LICENSE ${LicenseFileName}

  Function LicensePage_Show
    ${nsResize_LicensePage} ${MUI_CUSTOMIZE_ADDITIONAL_WIDTH} ${MUI_CUSTOMIZE_ADDITIONAL_HEIGHT}
  FunctionEnd
  !else
  !insertmacro MUI_PAGE_LICENSE ${LicenseFileName}
  !endif
  
!macroend

;-----------------------------------------------------------------------
; nsResize Directory page
;-----------------------------------------------------------------------

!macro MUI_PAGE_DIRECTORY_CUSTOMIZED

  !ifdef nsResize_DirectoryPage
  !define MUI_PAGE_CUSTOMFUNCTION_SHOW DirectoryPage_Show
  !insertmacro MUI_PAGE_DIRECTORY

  Function DirectoryPage_Show
    ${nsResize_DirectoryPage} ${MUI_CUSTOMIZE_ADDITIONAL_WIDTH} ${MUI_CUSTOMIZE_ADDITIONAL_HEIGHT}
  FunctionEnd
  !else
  !insertmacro MUI_PAGE_DIRECTORY  
  !endif

!macroend

;-----------------------------------------------------------------------
; nsResize Components page
;-----------------------------------------------------------------------

!macro MUI_PAGE_COMPONENTS_CUSTOMIZED

  !ifdef nsResize_ComponentsPage
  !define MUI_PAGE_CUSTOMFUNCTION_SHOW ComponentsPage_Show
  !insertmacro MUI_PAGE_COMPONENTS

  Function ComponentsPage_Show
    ${nsResize_ComponentsPage} ${MUI_CUSTOMIZE_ADDITIONAL_WIDTH} ${MUI_CUSTOMIZE_ADDITIONAL_HEIGHT}
  FunctionEnd
  !else
  !insertmacro MUI_PAGE_COMPONENTS
  !endif

!macroend

;-----------------------------------------------------------------------
; nsResize InstFiles page
;-----------------------------------------------------------------------

!macro MUI_PAGE_INSTFILES_CUSTOMIZED

  !ifdef nsResize_InstFilesPage
  !define MUI_CUSTOMFUNCTION_GUIINIT InstGUIInit

  Function InstGUIInit
    ${nsResize_Window} ${MUI_CUSTOMIZE_ADDITIONAL_WIDTH} ${MUI_CUSTOMIZE_ADDITIONAL_HEIGHT}
  FunctionEnd

  !define MUI_PAGE_CUSTOMFUNCTION_SHOW InstFilesPage_Show
  !insertmacro MUI_PAGE_INSTFILES

  Function InstFilesPage_Show
    ${nsResize_InstFilesPage} ${MUI_CUSTOMIZE_ADDITIONAL_WIDTH} ${MUI_CUSTOMIZE_ADDITIONAL_HEIGHT}
  FunctionEnd
  !else
  !insertmacro MUI_PAGE_INSTFILES
  !endif

!macroend

;-----------------------------------------------------------------------
; nsResize Finish page
;-----------------------------------------------------------------------

!macro MUI_PAGE_FINISH_CUSTOMIZED

  !ifdef nsResize_FinishPage
  !define MUI_PAGE_CUSTOMFUNCTION_SHOW FinishPage_Show
  !insertmacro MUI_PAGE_FINISH

  Function FinishPage_Show
    ${nsResize_FinishPage} ${MUI_CUSTOMIZE_ADDITIONAL_WIDTH} ${MUI_CUSTOMIZE_ADDITIONAL_HEIGHT}
  FunctionEnd
  !else
  !insertmacro MUI_PAGE_FINISH  
  !endif

!macroend

;-----------------------------------------------------------------------
; nsResize un.Confirm page
;-----------------------------------------------------------------------

!macro MUI_UNPAGE_CONFIRM_CUSTOMIZED

  !ifdef nsResize_ConfirmPage
  !define MUI_PAGE_CUSTOMFUNCTION_SHOW un.ConfirmPage_Show
  !insertmacro MUI_UNPAGE_CONFIRM

  Function un.ConfirmPage_Show
    ${nsResize_ConfirmPage} ${MUI_CUSTOMIZE_ADDITIONAL_WIDTH} ${MUI_CUSTOMIZE_ADDITIONAL_HEIGHT}
  FunctionEnd
  !else
  !insertmacro MUI_UNPAGE_CONFIRM
  !endif

!macroend

;-----------------------------------------------------------------------
; nsResize un.Components page
;-----------------------------------------------------------------------

!macro MUI_UNPAGE_COMPONENTS_CUSTOMIZED

  !ifdef nsResize_ComponentsPage
  !define MUI_PAGE_CUSTOMFUNCTION_SHOW un.ComponentsPage_Show
  !insertmacro MUI_UNPAGE_COMPONENTS

  Function un.ComponentsPage_Show
    ${nsResize_ComponentsPage} ${MUI_CUSTOMIZE_ADDITIONAL_WIDTH} ${MUI_CUSTOMIZE_ADDITIONAL_HEIGHT}
  FunctionEnd
  !else
  !insertmacro MUI_UNPAGE_COMPONENTS  
  !endif

!macroend

;-----------------------------------------------------------------------
; nsResize un.Finish page
;-----------------------------------------------------------------------

!macro MUI_UNPAGE_INSTFILES_CUSTOMIZED

  !ifdef nsResize_InstFilesPage
  !define MUI_CUSTOMFUNCTION_UNGUIINIT un.InstGUIInit

  Function un.InstGUIInit
    ${nsResize_Window} ${MUI_CUSTOMIZE_ADDITIONAL_WIDTH} ${MUI_CUSTOMIZE_ADDITIONAL_HEIGHT}
  FunctionEnd

  !define MUI_PAGE_CUSTOMFUNCTION_SHOW un.InstFilesPage_Show
  !insertmacro MUI_UNPAGE_INSTFILES

  Function un.InstFilesPage_Show
    ${nsResize_InstFilesPage} ${MUI_CUSTOMIZE_ADDITIONAL_WIDTH} ${MUI_CUSTOMIZE_ADDITIONAL_HEIGHT}
  FunctionEnd
  !else
  !insertmacro MUI_UNPAGE_INSTFILES  
  !endif

!macroend
